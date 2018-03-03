/*
 * drivers/usb/host/ehci-f_usb20ho.c
 *
 * Copyright (C) 2011-2012 FUJITSU SEMICONDUCTOR LIMITED
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/platform_device.h>
#include "f_usb20ho.h"
#include "linux/usb/f_usb/f_usb20ho_hub.h"
#include "linux/usb/f_usb/usb_otg_control.h"
#include <linux/udif/cache.h>

//#define FUSB_FUNC_TRACE

/* import usb disabled check function */
extern int usb_disabled(void);		/**<  */

/**
 * @brief ehci_otg_ops
 *
 *
*/
struct usb_otg_module_ops ehci_otg_ops;

#define FUSB_EHCI_MEM_FREE	0
#define FUSB_EHCI_MEM_MALLOC	1U

/**
 * @brief is_enable_ehci_irq
 *
 *
*/
static volatile bool is_enable_ehci_irq = true;

/**
 * @brief fusb_ehci_mem_init_flag
 *
 *
*/
static unsigned int fusb_ehci_mem_init_flag = FUSB_EHCI_MEM_FREE;

#define FUSB_EHCI_REMOVE_HCD	0
#define FUSB_EHCI_ADD_HCD	1U

/**
 * @brief fusb_ehci_add_hcd_flag
 *
 *
*/
static unsigned int fusb_ehci_add_hcd_flag = FUSB_EHCI_REMOVE_HCD;

#ifdef FUSB_FUNC_TRACE
#define fusb_func_trace(b)		printk("CALL %s %s(%d)\n",(b),__FUNCTION__,__LINE__)
#else
#define fusb_func_trace(b)
#endif

/************************************************************************************************/
/* Implementation of here to F_USB20HO device driver local function */

static void ehci_hcd_f_usb20ho_initialize_controller(struct usb_hcd *hcd)
{
	/*
	 * initialize F_USB20HO EHCI controller register
	 * [notice]:This is an original register which is not defined by EHCI
	 */
	set_f_usb20ho_register(hcd->regs, F_USB20HO_INSNREG01, (CONFIG_USB_EHCI_F_USB20HO_INSNREG01_REGISTER_DATA_OUT << 16) |
				CONFIG_USB_EHCI_F_USB20HO_INSNREG01_REGISTER_DATA_IN);
	set_f_usb20ho_register(hcd->regs, F_USB20HO_INSNREG02, CONFIG_USB_EHCI_F_USB20HO_INSNREG02_REGISTER_DATA);

	return;
}

static unsigned char ehci_hcd_f_usb20ho_is_4k_page_boundary_violated(struct usb_hcd *hcd, struct urb *urb)
{
	struct ehci_hcd *ehci;
	dma_addr_t dma_4k_align;
	unsigned short dma_4k_align_size;
	unsigned short maxpacket;

	ehci = hcd_to_ehci(hcd);

	switch (usb_pipetype(urb->pipe)) {
	case PIPE_CONTROL:
		dma_4k_align = urb->setup_dma & 0x0fffU;
		dma_4k_align_size = 0x1000U - dma_4k_align;
		if (dma_4k_align_size < 8U) {
			ehci_err(ehci, "4k page boundary is violated at setup_dma = 0x%08x.\n", urb->setup_dma);
			return 1U;
		}
		/* FALLTHROUGH */
		/* case PIPE_ISOCHRONOUS */
		/* case PIPE_BULK: */
		/* case PIPE_INTERRUPT */
	default:
		if (!urb->transfer_buffer_length)
			break;
		maxpacket = max_packet(usb_maxpacket(urb->dev, urb->pipe, !usb_pipein(urb->pipe)));
		if (maxpacket == 0){
			/* Failsafe for Division by zero in kernel. Correct parameters. */
			ehci_err(ehci, "ehci_hcd_f_usb20ho_is_4k_page_boundary_violated() Zero maxpacket \n");
			return 1U;
		}
		dma_4k_align = urb->transfer_dma & 0x0fffU;
		dma_4k_align_size = 0x1000U - dma_4k_align;
		if ((dma_4k_align_size < urb->transfer_buffer_length) && (dma_4k_align % maxpacket)) {
			ehci_err(ehci, "4k page boundary is violated at dma = 0x%08x, length = %u.\n", urb->transfer_dma, urb->transfer_buffer_length);
			return 1U;
		}
		break;
	}
	return 0;
}

static void ehci_hcd_f_usb20ho_start_ehci(struct ehci_hcd *ehci)
{
	fusb_func_trace("START");
	ehci->command = ehci_readl(ehci, &ehci->regs->command);
	ehci->command |= CMD_RUN;
	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	/* wait for ehci hcd start */
	while ((ehci_readl(ehci, &ehci->regs->status) & STS_HALT) == STS_HALT);
	fusb_func_trace("END");
}

static void ehci_hcd_f_usb20ho_stop_ehci(struct ehci_hcd *ehci)
{
	fusb_func_trace("START");
	ehci->command = ehci_readl(ehci, &ehci->regs->command);
	ehci->command &= ~CMD_RUN;
	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	/* wait for ehci hcd stop */
	while ((ehci_readl(ehci, &ehci->regs->status) & STS_HALT) != STS_HALT);
	fusb_func_trace("END");
}

static void ehci_hcd_f_usb20ho_wait_ohci_stopped(struct ehci_hcd *ehci)
{
	fusb_func_trace("START");
	/* wait for ohci hcd stop */
	while ((readl(ehci_to_hcd(ehci)->regs + 0x1004) & 0xC0) == 0x80)
		msleep(1);
	fusb_func_trace("END");
}

static int ehci_f_usb20ho_init(struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	u32			temp;
	int			retval;
	u32			hcc_params;
	struct ehci_qh_hw	*hw;

	fusb_func_trace("START");

	spin_lock_init(&ehci->lock);

	/*
	 * keep io watchdog by default, those good HCDs could turn off it later
	 */
	ehci->need_io_watchdog = 1;
	init_timer(&ehci->watchdog);
	ehci->watchdog.function = ehci_watchdog;
	ehci->watchdog.data = (unsigned long) ehci;

	init_timer(&ehci->iaa_watchdog);
	ehci->iaa_watchdog.function = ehci_iaa_watchdog;
	ehci->iaa_watchdog.data = (unsigned long) ehci;

	hcc_params = ehci_readl(ehci, &ehci->caps->hcc_params);

	/*
	 * hw default: 1K periodic list heads, one per frame.
	 * periodic_size can shrink by USBCMD update if hcc_params allows.
	 */
	ehci->periodic_size = DEFAULT_I_TDPS;
	INIT_LIST_HEAD(&ehci->cached_itd_list);
	INIT_LIST_HEAD(&ehci->cached_sitd_list);

	if (HCC_PGM_FRAMELISTLEN(hcc_params)) {
		/* periodic schedule size can be smaller than default */
		switch (EHCI_TUNE_FLS) {
		case 0: ehci->periodic_size = 1024; break;
		case 1: ehci->periodic_size = 512; break;
		case 2: ehci->periodic_size = 256; break;
		default:	BUG();
		}
	}
	if (fusb_ehci_mem_init_flag == FUSB_EHCI_MEM_FREE){
		if ((retval = ehci_mem_init(ehci, GFP_KERNEL)) < 0)
			return retval;
		fusb_ehci_mem_init_flag = FUSB_EHCI_MEM_MALLOC;
	}

	/* controllers may cache some of the periodic schedule ... */
	if (HCC_ISOC_CACHE(hcc_params))		// full frame cache
		ehci->i_thresh = 2 + 8;
	else					// N microframes cached
		ehci->i_thresh = 2 + HCC_ISOC_THRES(hcc_params);

	ehci->reclaim = NULL;
	ehci->next_uframe = -1;
	ehci->clock_frame = -1;

	/*
	 * dedicate a qh for the async ring head, since we couldn't unlink
	 * a 'real' qh without stopping the async schedule [4.8].  use it
	 * as the 'reclamation list head' too.
	 * its dummy is used in hw_alt_next of many tds, to prevent the qh
	 * from automatically advancing to the next td after short reads.
	 */
	ehci->async->qh_next.qh = NULL;
	hw = ehci->async->hw;
	hw->hw_next = QH_NEXT(ehci, ehci->async->qh_dma);
	hw->hw_info1 = cpu_to_hc32(ehci, QH_HEAD);
	hw->hw_token = cpu_to_hc32(ehci, QTD_STS_HALT);
	hw->hw_qtd_next = EHCI_LIST_END(ehci);
	ehci->async->qh_state = QH_STATE_LINKED;
	hw->hw_alt_next = QTD_NEXT(ehci, ehci->async->dummy->qtd_dma);

	/* clear interrupt enables, set irq latency */
	if (log2_irq_thresh < 0 || log2_irq_thresh > 6)
		log2_irq_thresh = 0;
	temp = 1 << (16 + log2_irq_thresh);
	if (HCC_PER_PORT_CHANGE_EVENT(hcc_params)) {
		ehci->has_ppcd = 1;
		ehci_dbg(ehci, "enable per-port change event\n");
		temp |= CMD_PPCEE;
	}
	if (HCC_CANPARK(hcc_params)) {
		/* HW default park == 3, on hardware that supports it (like
		 * NVidia and ALI silicon), maximizes throughput on the async
		 * schedule by avoiding QH fetches between transfers.
		 *
		 * With fast usb storage devices and NForce2, "park" seems to
		 * make problems:  throughput reduction (!), data errors...
		 */
		if (park) {
			park = min(park, (unsigned) 3);
			temp |= CMD_PARK;
			temp |= park << 8;
		}
		ehci_dbg(ehci, "park %d\n", park);
	}
	if (HCC_PGM_FRAMELISTLEN(hcc_params)) {
		/* periodic schedule size can be smaller than default */
		temp &= ~(3 << 2);
		temp |= (EHCI_TUNE_FLS << 2);
	}
	if (HCC_LPM(hcc_params)) {
		/* support link power management EHCI 1.1 addendum */
		ehci_dbg(ehci, "support lpm\n");
		ehci->has_lpm = 1;
		if (hird > 0xf) {
			ehci_dbg(ehci, "hird %d invalid, use default 0",
			hird);
			hird = 0;
		}
		temp |= hird << 24;
	}
	ehci->command = temp;

	/* Accept arbitrarily long scatter-gather lists */
	if (!(hcd->driver->flags & HCD_LOCAL_MEM))
		hcd->self.sg_tablesize = ~0;

	fusb_func_trace("END");
	return 0;
}

/* So far, it is implementation of F_USB20HO device driver local function            */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HO device driver upper layer interface            */

/*----------------------------------------------------------------------------*/
/**
 * @brief ehci_f_usb20ho_irq
 *
 *
 *
 * @param [in]  hcd
 *
 * @return
 * @retval IRQ_NONE
 * @retval IRQ_HANDLED
 */
/*----------------------------------------------------------------------------*/
static irqreturn_t ehci_f_usb20ho_irq(struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	u32			status, masked_status, pcd_status = 0, cmd;
	int			bh;

	fusb_func_trace("START");

	fvc2_h2xbb_complete_wait();

	spin_lock (&ehci->lock);

	status = ehci_readl(ehci, &ehci->regs->status);

	/* e.g. cardbus physical eject */
	if (status == ~(u32) 0) {
		ehci_dbg (ehci, "device removed\n");
		goto dead;
	}

	/* Shared IRQ? */
	masked_status = status & INTR_MASK;
	if (!masked_status || unlikely(hcd->state == HC_STATE_HALT)) {
		spin_unlock(&ehci->lock);
		return IRQ_NONE;
	}

	/* clear (just) interrupts */
	ehci_writel(ehci, masked_status, &ehci->regs->status);
	cmd = ehci_readl(ehci, &ehci->regs->command);
	bh = 0;

#ifdef	VERBOSE_DEBUG
	/* unrequested/ignored: Frame List Rollover */
	dbg_status (ehci, "irq", status);
#endif

	/* INT, ERR, and IAA interrupt rates can be throttled */

	/* normal [4.15.1.2] or error [4.15.1.1] completion */
	if (likely ((status & (STS_INT|STS_ERR)) != 0)) {
		if (likely ((status & STS_ERR) == 0))
			COUNT (ehci->stats.normal);
		else
			COUNT (ehci->stats.error);
		bh = 1;
	}

	/* complete the unlinking of some qh [4.15.2.3] */
	if (status & STS_IAA) {

		fusb_func_trace("irq STS_IAA \n");

		/* guard against (alleged) silicon errata */
		if (cmd & CMD_IAAD) {
			ehci_writel(ehci, cmd & ~CMD_IAAD,
					&ehci->regs->command);
			ehci_dbg(ehci, "IAA with IAAD still set?\n");
		}
		if (ehci->reclaim) {
			COUNT(ehci->stats.reclaim);
			end_unlink_async(ehci);
		} else
			ehci_dbg(ehci, "IAA with nothing to reclaim?\n");
	}

	/* remote wakeup [4.3.1] */
	if (status & STS_PCD) {
		unsigned	i = HCS_N_PORTS (ehci->hcs_params);
		u32		ppcd = 0;

		fusb_func_trace("irq STS_PCD \n");

		/* kick root hub later */
		pcd_status = status;

		/* resume root hub? */
		if (!(cmd & CMD_RUN))
			usb_hcd_resume_root_hub(hcd);

		/* get per-port change detect bits */
		if (ehci->has_ppcd)
			ppcd = status >> 16;

		while (i--) {
			int pstatus;

			/* leverage per-port change bits feature */
			if (ehci->has_ppcd && !(ppcd & (1 << i)))
				continue;
			pstatus = ehci_readl(ehci,
					 &ehci->regs->port_status[i]);

			if (pstatus & PORT_OWNER)
				continue;
			if (!(test_bit(i, &ehci->suspended_ports) &&
					((pstatus & PORT_RESUME) ||
						!(pstatus & PORT_SUSPEND)) &&
					(pstatus & PORT_PE) &&
					ehci->reset_done[i] == 0))
				continue;

			/* start 20 msec resume signaling from this port,
			 * and make khubd collect PORT_STAT_C_SUSPEND to
			 * stop that signaling.  Use 5 ms extra for safety,
			 * like usb_port_resume() does.
			 */
			ehci->reset_done[i] = jiffies + msecs_to_jiffies(25);
			ehci_dbg (ehci, "port %d remote wakeup\n", i + 1);
			mod_timer(&hcd->rh_timer, ehci->reset_done[i]);
		}
	}

	/* PCI errors [4.15.2.4] */
	if (unlikely ((status & STS_FATAL) != 0)) {

		fusb_func_trace("irq STS_FATAL \n");

		ehci_err(ehci, "fatal error\n");
		dbg_cmd(ehci, "fatal", cmd);
		dbg_status(ehci, "fatal", status);
		ehci_halt(ehci);
#ifdef CONFIG_ARCH_EMXX
		emxx_hc_vbus_control(0);
#endif	/* CONFIG_ARCH_EMXX */
dead:
		ehci_reset(ehci);
		ehci_writel(ehci, 0, &ehci->regs->configured_flag);
		usb_hc_died(hcd);
		/* generic layer kills/unlinks all urbs, then
		 * uses ehci_stop to clean up the rest
		 */
		bh = 1;
	}

	if (bh)
		ehci_work (ehci);
	spin_unlock (&ehci->lock);
	if (pcd_status)
		usb_hcd_poll_rh_status(hcd);

#ifdef CONFIG_ARCH_EMXX
	if (unlikely ((status & STS_FATAL) != 0)) {
		disable_irq(INT_USB_OCI);
		emxx_hc_fatal_recovery(hcd);
	}
#endif	/* CONFIG_ARCH_EMXX */

	fusb_func_trace("END");
	return IRQ_HANDLED;
}

static int ehci_f_usb20ho_reset(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int result;

	fusb_func_trace("START");

	ehci_dbg(ehci, "%s() is started.\n", __FUNCTION__);

	/* halt ehci */
	result = ehci_halt(ehci);
	if (result) {
		ehci_err(ehci, "ehci_halt() is failed at %d.", result);
		ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);
		return result;
	}

	/* initialize ehci */
	result = ehci_f_usb20ho_init(hcd);
	if (result) {
		ehci_err(ehci, "ehci_f_usb20ho_init() is failed at %d.", result);
		ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);
		return result;
	}

	/* reset ehci */
	result = ehci_reset(ehci);
	if (result) {
		ehci_err(ehci, "ehci_reset() is failed at %d.", result);
		ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);
		return result;
	}

	/* power-off port */
	ehci_port_power(ehci, 0);

	ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");
	return 0;
}


static int ehci_f_usb20ho_start(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci;
	int result;

	fusb_func_trace("START");

	ehci = hcd_to_ehci(hcd);

	ehci_dbg(ehci, "%s() is started.\n", __FUNCTION__);

	/* run ehci */
	result = ehci_run(hcd);
	if (result) {
		ehci_err(ehci, "ehci_run() is failed at %d.", result);
		ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);
		return result;
	}

	ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");
	return 0;
}

static int ehci_f_usb20ho_pci_suspend(struct usb_hcd *hcd, bool do_wakeup)
{
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	unsigned long		flags;
	int			rc = 0;

	fusb_func_trace("START");

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible.  The PM and USB cores make sure that
	 * the root hub is either suspended or stopped.
	 */
	ehci_prepare_ports_for_controller_suspend(ehci, do_wakeup);
	spin_lock_irqsave (&ehci->lock, flags);
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	spin_unlock_irqrestore (&ehci->lock, flags);

	// could save FLADJ in case of Vaux power loss
	// ... we'd only use it to handle clock skew

	fusb_func_trace("END");
	return rc;
}

static int ehci_f_usb20ho_pci_resume(struct usb_hcd *hcd, bool hibernated)
{
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	fusb_func_trace("START");

	fusb_otg_resume_initial_set();

	if (time_before(jiffies, ehci->next_statechange))
		msleep(100);

	/* initialize F_USB20HO controller */
	ehci_hcd_f_usb20ho_initialize_controller(hcd);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	usb_root_hub_lost_power(hcd->self.root_hub);

	/* Else reset, to cope with power loss or flush-to-storage
	 * style "resume" having let BIOS kick in during reboot.
	 */
	ehci_f_usb20ho_reset(hcd);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;
	fusb_func_trace("END");
	return 0;
}

static int ehci_f_usb20ho_bus_resume (struct usb_hcd *hcd)
{
	int rc = 0;

	fusb_func_trace("START");

	rc = ehci_bus_resume(hcd);

#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	fusb_otg_host_resume_finish(FUSB_OTG_HOST_EHCI);
#endif

	fusb_func_trace("END");

	return rc;
}


static void ehci_f_usb20ho_stop(struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);

	fusb_func_trace("START");

	ehci_dbg (ehci, "stop\n");

	/* no more interrupts ... */
	del_timer_sync (&ehci->watchdog);
	del_timer_sync(&ehci->iaa_watchdog);

	spin_lock_irq(&ehci->lock);
	if (HC_IS_RUNNING (hcd->state))
		ehci_quiesce (ehci);

	ehci_silence_controller(ehci);
	ehci_reset (ehci);
	spin_unlock_irq(&ehci->lock);

	remove_companion_file(ehci);
	remove_debug_files (ehci);

	/* root hub is shut down separately (first, when possible) */
	spin_lock_irq (&ehci->lock);
	if (ehci->async)
		ehci_work (ehci);
	spin_unlock_irq (&ehci->lock);
	if (fusb_ehci_mem_init_flag == FUSB_EHCI_MEM_MALLOC){
		ehci_mem_cleanup (ehci);
		fusb_ehci_mem_init_flag = FUSB_EHCI_MEM_FREE;
	}

	if (ehci->amd_pll_fix == 1)
		usb_amd_dev_put();

#ifdef	EHCI_STATS
	ehci_dbg (ehci, "irq normal %ld err %ld reclaim %ld (lost %ld)\n",
		ehci->stats.normal, ehci->stats.error, ehci->stats.reclaim,
		ehci->stats.lost_iaa);
	ehci_dbg (ehci, "complete %ld unlink %ld\n",
		ehci->stats.complete, ehci->stats.unlink);
#endif

	dbg_status (ehci, "ehci_stop completed",
		    ehci_readl(ehci, &ehci->regs->status));
	fusb_func_trace("END");
}

static int ehci_f_usb20ho_urb_enqueue (
	struct usb_hcd	*hcd,
	struct urb	*urb,
	gfp_t		mem_flags
) {
	struct ehci_hcd *ehci;
	int result;

	fusb_func_trace("START");

	if ((hcd == NULL)||(urb == NULL)){
		return -EINVAL;
	}

	ehci = hcd_to_ehci(hcd);

	ehci_dbg(ehci, "%s() is started.\n", __FUNCTION__);

	/* check 4k page boundary violation */
	if (ehci_hcd_f_usb20ho_is_4k_page_boundary_violated(hcd, urb)) {
		ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);
		return -EBADR;
	}

	/* enqueue ehci urb */
	result = ehci_urb_enqueue(hcd, urb, mem_flags);
	if (result) {
		ehci_err(ehci, "ehci_urb_enqueue() is failed at %d.", result);
		ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);
		return result;
	}

	ehci_dbg(ehci, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");
	return 0;
}

static int
ehci_f_usb20ho_hub_status_data (struct usb_hcd *hcd , char *buf)
{
	struct ehci_hcd	*ehci = hcd_to_ehci (hcd);
	u32		temp, status = 0;
	u32		mask;
	int		ports, i, retval = 1;
	unsigned long	flags;
	u32		ppcd = 0;

	fusb_func_trace("START");

	if ((hcd == NULL) || (buf == NULL)){
		return -EINVAL;
	}

	/* if !USB_SUSPEND, root hub timers won't get shut down ... */
	if (!HC_IS_RUNNING(hcd->state)){
		return 0;
	}

	/* init status to no-changes */
	buf [0] = 0;
	ports = HCS_N_PORTS (ehci->hcs_params);
	if (ports > 7) {
		buf [1] = 0;
		retval++;
	}

	/* Some boards (mostly VIA?) report bogus overcurrent indications,
	 * causing massive log spam unless we completely ignore them.  It
	 * may be relevant that VIA VT8235 controllers, where PORT_POWER is
	 * always set, seem to clear PORT_OCC and PORT_CSC when writing to
	 * PORT_POWER; that's surprising, but maybe within-spec.
	 */
	if (!ignore_oc)
		mask = PORT_CSC | PORT_PEC | PORT_OCC;
	else
		mask = PORT_CSC | PORT_PEC;
	// PORT_RESUME from hardware ~= PORT_STAT_C_SUSPEND

	/* no hub change reports (bit 0) for now (power, ...) */

	/* port N changes (bit N)? */
	spin_lock_irqsave (&ehci->lock, flags);

	for (i = 0; i < ports; i++) {
		/* leverage per-port change bits feature */
		if (ehci->has_ppcd && !(ppcd & (1 << i)))
			continue;
		temp = ehci_readl(ehci, &ehci->regs->port_status [i]);

		/*
		 * Return status information even for ports with OWNER set.
		 * Otherwise khubd wouldn't see the disconnect event when a
		 * high-speed device is switched over to the companion
		 * controller by the user.
		 */

		if ((temp & mask) != 0 || test_bit(i, &ehci->port_c_suspend)
				|| (ehci->reset_done[i] && time_after_eq(
					jiffies, ehci->reset_done[i]))) {
			if (i < 7)
			    buf [0] |= 1 << (i + 1);
			else
			    buf [1] |= 1 << (i - 7);
			status = STS_PCD;
		}

		if (temp & (PORT_OCC | PORT_OC)){
			fusb_otg_notify_vbuserror();
		}

		if (temp & PORT_CSC){
			if (temp & PORT_CONNECT)
				fusb_otg_notify_connect(FUSB_EVENT_CONNECT);
			else
				fusb_otg_notify_connect(FUSB_EVENT_DISCONNECT);
		}
	}
	/* FIXME autosuspend idle root hubs */
	spin_unlock_irqrestore (&ehci->lock, flags);

	fusb_func_trace("END");
	return status ? retval : 0;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief ehci_f_usb20ho_check_reset_complete
 *
 * ehci-hub.c(ehci_check_reset_complete) implementation
 *
 * @param [in]  ehci
 * @param [in]  index
 * @param [in]  status_reg
 * @param [in]  port_status
 *
 * @return port_status
 * @retval port_status
 */
/*----------------------------------------------------------------------------*/
static int ehci_f_usb20ho_check_reset_complete (
	struct ehci_hcd	*ehci,
	int		index,
	u32 __iomem	*status_reg,
	int		port_status
) {
	if (!(port_status & PORT_CONNECT))
		return port_status;

	/* if reset finished and it's still not enabled -- handoff */
	if (!(port_status & PORT_PE)) {

		/* with integrated TT, there's nobody to hand it to! */
		if (ehci_is_TDI(ehci)) {
			ehci_dbg (ehci,
				"Failed to enable port %d on root hub TT\n",
				index+1);
			return port_status;
		}

#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
		/*
		 * [notice]
		 * When a full speed device is connected,
		 * it is necessary to stop EHCI operation.
		 */
		ehci_dbg(ehci, "[CONFLICT_GUARD]ehci stop.\n");
		ehci_hcd_f_usb20ho_stop_ehci(ehci);
		ehci_dbg(ehci, "[CONFLICT_GUARD]ehci stop completed.\n");
#endif

		ehci_dbg (ehci, "port %d full speed --> companion\n",
			index + 1);

		// what happens if HCS_N_CC(params) == 0 ?
		port_status |= PORT_OWNER;
		port_status &= ~PORT_RWC_BITS;
		ehci_writel(ehci, port_status, status_reg);

		/* ensure 440EPX ohci controller state is operational */
		if (ehci->has_amcc_usb23)
			set_ohci_hcfs(ehci, 1);
	} else {
		ehci_dbg (ehci, "port %d high speed\n", index + 1);
#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
		/*
		 * [notice]
		 * When a device connect staus is changed,
		 * it is necessary to check OHCI operation is stopped.
		 * This operation should be performed
		 * before the spin_lock.
		 * Because "ehci_hcd_f_usb20ho_wait_ohci_stopped"
		 * is using msleep().
		 */
		ehci_dbg(ehci, "[CONFLICT_GUARD]wait ohci stop.\n");
		ehci_hcd_f_usb20ho_wait_ohci_stopped(ehci);
		ehci_dbg(ehci, "[CONFLICT_GUARD]wait ohci stop completed.\n");

		/*
		 * [notice]
		 * When a device connect staus is changed,
		 * It is necessary to restart EHCI operation.
		 */
		ehci_dbg(ehci, "[CONFLICT GUARD]ehci start.\n");
		ehci_hcd_f_usb20ho_start_ehci(ehci);
		ehci_dbg(ehci, "[CONFLICT GUARD]ehci start completed.\n");
#endif
		/* ensure 440EPx ohci controller state is suspended */
		if (ehci->has_amcc_usb23)
			set_ohci_hcfs(ehci, 0);
	}

	return port_status;
}

static int ehci_f_usb20ho_hub_control(
	struct usb_hcd	*hcd,
	u16		typeReq,
	u16		wValue,
	u16		wIndex,
	char		*buf,
	u16		wLength
) {
	struct ehci_hcd	*ehci = hcd_to_ehci (hcd);
	int		ports = HCS_N_PORTS (ehci->hcs_params);
	u32 __iomem	*status_reg = &ehci->regs->port_status[
				(wIndex & 0xff) - 1];
	u32 __iomem	*hostpc_reg = NULL;
	u32		temp, temp1, status;
	unsigned long	flags;
	int		retval = 0;
	int		hub_control_flag = 0;
	struct usb_hcd	*_hcd = hcd;
	u16		_typeReq = typeReq;
	u16		_wValue = wValue;
	u16		_wIndex = wIndex;
	char		*_buf = buf;
	u16		_wLength = wLength;

	fusb_func_trace("START");

	if ((hcd == NULL) || (buf == NULL))
		return -EINVAL;

	if (ehci->has_hostpc)
		hostpc_reg = (u32 __iomem *)((u8 *)ehci->regs
				+ HOSTPC0 + 4 * ((wIndex & 0xff) - 1));

	spin_lock_irqsave (&ehci->lock, flags);
	switch (typeReq) {
	case F_USB20HO_ClearPortFeature:
		temp = ehci_readl(ehci, status_reg);
		switch (wValue) {
		case F_USB20HO_PORT_FEAT_TEST_SUSPEND:
			ehci_writel(ehci, temp | PORT_RESUME, status_reg);
			break;
		default:
			temp1 = temp & ~F_USB20HO_PORT_TEST_MASK;
			ehci_writel(ehci, temp1, status_reg);
			break;
		}
		break;
	case F_USB20HO_SetPortFeature:
		temp = ehci_readl(ehci, status_reg);
		switch (wValue) {
		case F_USB20HO_PORT_FEAT_EP_NO_DMA:
			break;
		case F_USB20HO_PORT_FEAT_TEST_SE0:
			temp1 = (temp & ~F_USB20HO_PORT_TEST_MASK) | F_USB20HO_PORT_TEST_SE0_NAK;
			ehci_writel(ehci, temp1, status_reg);
			break;
		case F_USB20HO_PORT_FEAT_TEST_J:
			temp1 = (temp & ~F_USB20HO_PORT_TEST_MASK) | F_USB20HO_PORT_TEST_J_STATE;
			ehci_writel(ehci, temp1, status_reg);
			break;
		case F_USB20HO_PORT_FEAT_TEST_K:
			temp1 = (temp & ~F_USB20HO_PORT_TEST_MASK) | F_USB20HO_PORT_TEST_K_STATE;
			ehci_writel(ehci, temp1, status_reg);
			break;
		case F_USB20HO_PORT_FEAT_TEST_PACKET:
			temp1 = (temp & ~F_USB20HO_PORT_TEST_MASK) | F_USB20HO_PORT_TEST_PACKET;
			ehci_writel(ehci, temp1, status_reg);
			break;
		case F_USB20HO_PORT_FEAT_TEST_SETUP:
			break;
		case F_USB20HO_PORT_FEAT_TEST_IN:
			break;
		case F_USB20HO_PORT_FEAT_TEST_FORCE_ENABLE:
			temp1 = (temp & ~F_USB20HO_PORT_TEST_MASK) | F_USB20HO_PORT_TEST_FORCE_ENABLE;
			ehci_writel(ehci, temp1, status_reg);
			break;
		case F_USB20HO_PORT_FEAT_TEST_SUSPEND:
			ehci_writel(ehci, temp | PORT_SUSPEND, status_reg);
			break;
		default:
			hub_control_flag = 1;
			break;
		}
		break;
	case ClearPortFeature:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		temp = ehci_readl(ehci, status_reg);

		/*
		 * Even if OWNER is set, so the port is owned by the
		 * companion controller, khubd needs to be able to clear
		 * the port-change status bits (especially
		 * USB_PORT_STAT_C_CONNECTION).
		 */

		switch (wValue) {
		case USB_PORT_FEAT_C_CONNECTION:
			if (ehci->has_lpm) {
				/* clear PORTSC bits on disconnect */
				temp &= ~PORT_LPM;
				temp &= ~PORT_DEV_ADDR;
			}
			ehci_writel(ehci, (temp & ~PORT_RWC_BITS) | PORT_CSC,
					status_reg);
			ehci_readl(ehci, &ehci->regs->command);	/* unblock posted write */
			break;
		default:
			hub_control_flag = 1;
			break;
		}
		break;
	case GetPortStatus:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		status = 0;
		temp = ehci_readl(ehci, status_reg);

		// wPortChange bits
		if (temp & PORT_CSC)
			status |= USB_PORT_STAT_C_CONNECTION << 16;
		if (temp & PORT_PEC)
			status |= USB_PORT_STAT_C_ENABLE << 16;

		if ((temp & PORT_OCC) && !ignore_oc){
			status |= USB_PORT_STAT_C_OVERCURRENT << 16;

			/*
			 * Hubs should disable port power on over-current.
			 * However, not all EHCI implementations do this
			 * automatically, even if they _do_ support per-port
			 * power switching; they're allowed to just limit the
			 * current.  khubd will turn the power back on.
			 */
			if ((temp & PORT_OC) && HCS_PPC(ehci->hcs_params)) {
				ehci_writel(ehci,
					temp & ~(PORT_RWC_BITS | PORT_POWER),
					status_reg);
				temp = ehci_readl(ehci, status_reg);
			}
		}

		/* whoever resumes must GetPortStatus to complete it!! */
		if (temp & PORT_RESUME) {

			/* Remote Wakeup received? */
			if (!ehci->reset_done[wIndex]) {
				/* resume signaling for 20 msec */
				ehci->reset_done[wIndex] = jiffies
						+ msecs_to_jiffies(20);
				/* check the port again */
				mod_timer(&ehci_to_hcd(ehci)->rh_timer,
						ehci->reset_done[wIndex]);
			}

			/* resume completed? */
			else if (time_after_eq(jiffies,
					ehci->reset_done[wIndex])) {
				set_bit(wIndex, &ehci->port_c_suspend);
				ehci->reset_done[wIndex] = 0;

				/* stop resume signaling */
				temp = ehci_readl(ehci, status_reg);
				ehci_writel(ehci,
					temp & ~(PORT_RWC_BITS | PORT_RESUME),
					status_reg);
				retval = handshake(ehci, status_reg,
					   PORT_RESUME, 0, 2000 /* 2msec */);
				if (retval != 0) {
					ehci_err(ehci,
						"port %d resume error %d\n",
						wIndex + 1, retval);
					goto error;
				}
				temp &= ~(PORT_SUSPEND|PORT_RESUME|(3<<10));
			}
		}

		/* whoever resets must GetPortStatus to complete it!! */
		if ((temp & PORT_RESET)
				&& time_after_eq(jiffies,
					ehci->reset_done[wIndex])) {
			status |= 1 << USB_PORT_FEAT_C_RESET;
			ehci->reset_done [wIndex] = 0;

			/* force reset to complete */
			ehci_writel(ehci, temp & ~(PORT_RWC_BITS | PORT_RESET),
					status_reg);
			/* REVISIT:  some hardware needs 550+ usec to clear
			 * this bit; seems too long to spin routinely...
			 */
			retval = handshake(ehci, status_reg,
					PORT_RESET, 0, 1000);
			if (retval != 0) {
				ehci_err (ehci, "port %d reset error %d\n",
					wIndex + 1, retval);
				goto error;
			}
			/* see what we found out */
			temp = ehci_f_usb20ho_check_reset_complete (ehci, wIndex, status_reg,
					ehci_readl(ehci, status_reg));
		}

		if (!(temp & (PORT_RESUME|PORT_RESET)))
			ehci->reset_done[wIndex] = 0;

		/* transfer dedicated ports to the companion hc */
		if ((temp & PORT_CONNECT) &&
				test_bit(wIndex, &ehci->companion_ports)) {
			temp &= ~PORT_RWC_BITS;
			temp |= PORT_OWNER;
			ehci_writel(ehci, temp, status_reg);
			ehci_dbg(ehci, "port %d --> companion\n", wIndex + 1);
			temp = ehci_readl(ehci, status_reg);
		}

		/*
		 * Even if OWNER is set, there's no harm letting khubd
		 * see the wPortStatus values (they should all be 0 except
		 * for PORT_POWER anyway).
		 */

		if (temp & PORT_CONNECT) {
			status |= USB_PORT_STAT_CONNECTION;
			// status may be from integrated TT
			if (ehci->has_hostpc) {
				temp1 = ehci_readl(ehci, hostpc_reg);
				status |= ehci_port_speed(ehci, temp1);
			} else
				status |= ehci_port_speed(ehci, temp);
		}
		if (temp & PORT_PE)
			status |= USB_PORT_STAT_ENABLE;

		/* maybe the port was unsuspended without our knowledge */
		if (temp & (PORT_SUSPEND|PORT_RESUME)) {
			status |= USB_PORT_STAT_SUSPEND;
		} else if (test_bit(wIndex, &ehci->suspended_ports)) {
			clear_bit(wIndex, &ehci->suspended_ports);
			ehci->reset_done[wIndex] = 0;
			if (temp & PORT_PE)
				set_bit(wIndex, &ehci->port_c_suspend);
		}

		if (temp & PORT_OC)
			status |= USB_PORT_STAT_OVERCURRENT;
		if (temp & PORT_RESET)
			status |= USB_PORT_STAT_RESET;
		if (temp & PORT_POWER)
			status |= USB_PORT_STAT_POWER;
		if (test_bit(wIndex, &ehci->port_c_suspend))
			status |= USB_PORT_STAT_C_SUSPEND << 16;

#ifndef	VERBOSE_DEBUG
	if (status & ~0xffff)	/* only if wPortChange is interesting */
#endif
		dbg_port (ehci, "GetStatus", wIndex + 1, temp);
		put_unaligned_le32(status, buf);
		break;
	default:
		hub_control_flag = 1;
		break;
	}
error:
	spin_unlock_irqrestore (&ehci->lock, flags);

	if (hub_control_flag) {
		retval = ehci_hub_control(_hcd, _typeReq, _wValue, _wIndex, _buf, _wLength);
	}

	fusb_func_trace("END");
	return retval;
}

static void ehci_f_usb20ho_enable_irq (bool isEnable)
{
	fusb_func_trace("START");

	if (isEnable) {
		if (!is_enable_ehci_irq)
			enable_irq(F_USB_HO_EHCI_IRQ);
	} else {
		if (is_enable_ehci_irq)
			disable_irq(F_USB_HO_EHCI_IRQ);
	}

	is_enable_ehci_irq = isEnable;

	fusb_func_trace("END");
}
/* So far, it is implementation of F_USB20HO device driver upper layer interface        */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HO device driver probing / removing                */
static void ehci_hcd_f_usb20ho_set_soft_reset(struct usb_hcd *hcd)
{
	volatile u32 status;
	volatile u32 phy_reg_value;

	fusb_func_trace("START");

	/* store phy setting */
	phy_reg_value = get_f_usb20ho_register(hcd->regs, F_USB20HO_OTHER_BASE + 0x0C);

	/* soft reset */
	status = get_f_usb20ho_register(hcd->regs, F_USB20HO_PHYMODESETTING1);
	status |= (u32)F_USB20HO_RESET;
	set_f_usb20ho_register(hcd->regs, F_USB20HO_PHYMODESETTING1, status);
	msleep(F_USB_HO_PHY_RESET_TIME);

	status &= ~(u32)F_USB20HO_RESET;
	set_f_usb20ho_register(hcd->regs, F_USB20HO_PHYMODESETTING1, status);
	msleep(F_USB_HO_PHY_RESET_TIME);

	/* phy setting */
	set_f_usb20ho_register(hcd->regs, F_USB20HO_OTHER_BASE + 0x0C, phy_reg_value);

	fusb_func_trace("END");
}

void ehci_hcd_f_usb20ho_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	f_usb20ho_unmap_urb_for_dma(hcd, urb);
	
	return;
}

static int ehci_hcd_f_usb20ho_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
			    gfp_t mem_flags)
{
	int ret = 0;

	ret = f_usb20ho_map_urb_for_dma(hcd, urb, mem_flags);
	
	return ret;
}

static int ehci_hcd_f_usb20ho_otg_probe(void *data)
{
	int result = 0;
	int f_usb20ho_irq;
	struct usb_hcd *hcd = (struct usb_hcd *)data;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;

	fusb_func_trace("START");

	ehci_hcd_f_usb20ho_set_soft_reset(hcd);

	/* initialize F_USB20HO controller */
	ehci_hcd_f_usb20ho_initialize_controller(hcd);

	/* get an IRQ for a F_USB20HO device */
	f_usb20ho_irq = F_USB_HO_EHCI_IRQ;

	/* add hcd device */
	spin_lock_irqsave(&ehci->lock, flags);
	if (fusb_ehci_add_hcd_flag == FUSB_EHCI_REMOVE_HCD){
		spin_unlock_irqrestore(&ehci->lock, flags);
		result = usb_add_hcd(hcd, f_usb20ho_irq, 0);
		if (result) {
			usb_put_hcd(hcd);
			return result;
		}
		spin_lock_irqsave(&ehci->lock, flags);
		fusb_ehci_add_hcd_flag = FUSB_EHCI_ADD_HCD;
	}
	spin_unlock_irqrestore(&ehci->lock, flags);

	fusb_func_trace("END");
	return result;
}

static int ehci_hcd_f_usb20ho_otg_remove(void *data)
{
	struct usb_hcd *hcd = (struct usb_hcd *)data;
	struct ehci_hcd *ehci;
	unsigned long flags;
	u32 status  = 0;

	fusb_func_trace("START");

	ehci = hcd_to_ehci(hcd);

	/* remove hcd device */
	spin_lock_irqsave(&ehci->lock, flags);

	if (fusb_ehci_add_hcd_flag == FUSB_EHCI_ADD_HCD){
		fusb_ehci_add_hcd_flag = FUSB_EHCI_REMOVE_HCD;
		spin_unlock_irqrestore(&ehci->lock, flags);
		usb_remove_hcd(hcd);
		spin_lock_irqsave(&ehci->lock, flags);
	}
	spin_unlock_irqrestore(&ehci->lock, flags);

	status = ehci_readl(ehci, (__u32 __iomem *)&ehci->regs->port_status);
	status = status & ~(u32)PORT_POWER;
	ehci_writel(ehci, status, (__u32 __iomem *)&ehci->regs->port_status);

	fusb_func_trace("END");
	return 0;
}

static int ehci_hcd_f_usb20ho_otg_resume(void *data)
{
	struct usb_hcd *hcd = (struct usb_hcd *)data;

	fusb_func_trace("START");

	ehci_hcd_f_usb20ho_set_soft_reset(hcd);

	fusb_func_trace("END");
	return 0;
}

static int ehci_hcd_f_usb20ho_probe(struct platform_device *pdev)
{
	static const struct hc_driver ehci_f_usb20ho_hc_driver = {
		.description = hcd_name,
		.product_desc = "F_USB20HO EHCI",
		.hcd_priv_size = sizeof(struct ehci_hcd),
		.irq = ehci_f_usb20ho_irq,
		.flags = HCD_MEMORY | HCD_USB2,
		.reset = ehci_f_usb20ho_reset,
		.start = ehci_f_usb20ho_start,
		.pci_suspend = ehci_f_usb20ho_pci_suspend,
		.pci_resume = ehci_f_usb20ho_pci_resume,
		.stop = ehci_f_usb20ho_stop,
		.shutdown = ehci_shutdown,
		.get_frame_number = ehci_get_frame,
		.urb_enqueue = ehci_f_usb20ho_urb_enqueue,
		.urb_dequeue = ehci_urb_dequeue,
		.map_urb_for_dma = ehci_hcd_f_usb20ho_map_urb_for_dma,
		.unmap_urb_for_dma = ehci_hcd_f_usb20ho_unmap_urb_for_dma,
		.endpoint_disable = ehci_endpoint_disable,
		.hub_status_data = ehci_f_usb20ho_hub_status_data,
		.hub_control = ehci_f_usb20ho_hub_control,
#ifndef CONFIG_PM
		.bus_suspend = NULL,
		.bus_resume = NULL,
#else
		.bus_suspend = ehci_bus_suspend,
	#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
		.bus_resume = ehci_f_usb20ho_bus_resume,
	#else
		.bus_resume = ehci_bus_resume,
	#endif
#endif
		.start_port_reset = NULL,
		.relinquish_port = ehci_relinquish_port,
		.port_handed_over = ehci_port_handed_over,
	};

	void __iomem *f_usb20ho_register_base_address;
	int f_usb20ho_irq;
	struct usb_hcd *hcd = NULL;
	struct ehci_hcd *ehci;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!pdev))
		return -EINVAL;

	dev_dbg(&pdev->dev, "%s() is started.\n", __FUNCTION__);

	/* driver version output */
	dev_dbg(&pdev->dev, "F_USB20HO EHCI driver version is %s.\n", F_USB20HO_CONFIG_EHCI_DRIVER_VERSION);

	/* check usb disabled */
	if (usb_disabled()) {
		dev_err(&pdev->dev, "usb_disabled() is true.\n");
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* create hcd */
	hcd = usb_create_hcd(&ehci_f_usb20ho_hc_driver, &pdev->dev, "f_usb20ho_ehci");
	if (!hcd) {
		dev_err(&pdev->dev, "usb_create_hcd() is failed.\n");
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENOMEM;
	}

	/* get a register base address for a F_USB20HO device */
	f_usb20ho_register_base_address = (void __iomem *)fusb_otg_get_base_addr(FUSB_ADDR_TYPE_HOST);

	if (!f_usb20ho_register_base_address) {
		dev_err(&pdev->dev, "fusb_otg_get_base_addr() is failed.\n");
		usb_put_hcd(hcd);
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* get an IRQ for a F_USB20HO device */
	f_usb20ho_irq = F_USB_HO_EHCI_IRQ;

	/* setup the hcd structure parameter */
	hcd->regs = f_usb20ho_register_base_address;

	g_fvc2_h2xbb_regs = (void __iomem *)fusb_otg_get_base_addr(FUSB_ADDR_TYPE_H2XBB);

	if (!g_fvc2_h2xbb_regs) {
		dev_err(&pdev->dev, "fusb_otg_get_base_addr() is failed.\n");
		usb_put_hcd(hcd);
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* setup the ehci structure parameter */
	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(ehci , ehci_readl(ehci, &ehci->caps->hc_capbase));
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);
	ehci->hcs_params &= ~((0xfU << 8) | (0xfU << 0));                           /* N_PCC = 1, N_PORTS = 1 */
	ehci->hcs_params |= ((1U << 8) | (1U << 0));
	ehci->sbrn = 0x20U;

	/* setup the private data of a device driver */
	platform_set_drvdata(pdev, hcd);

	ehci_otg_ops.pdev   = pdev;
	ehci_otg_ops.data   = (void *)hcd;
	ehci_otg_ops.probe  = ehci_hcd_f_usb20ho_otg_probe;
	ehci_otg_ops.remove = ehci_hcd_f_usb20ho_otg_remove;
	ehci_otg_ops.resume = ehci_hcd_f_usb20ho_otg_resume;
	ehci_otg_ops.enable_irq = ehci_f_usb20ho_enable_irq;

	fusb_otg_host_ehci_bind(&ehci_otg_ops);

	fusb_func_trace("END");
	return 0;
}

static int ehci_hcd_f_usb20ho_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!pdev))
		return -EINVAL;

	dev_dbg(&pdev->dev, "%s() is started.\n", __FUNCTION__);

	/* get hcd data */
	hcd = platform_get_drvdata(pdev);
	if (!hcd) {
		dev_err(&pdev->dev, "platform_get_drvdata() is failed.\n");
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* remove hcd device */
	ehci = hcd_to_ehci(hcd);
	spin_lock_irqsave(&ehci->lock, flags);

	if (fusb_ehci_add_hcd_flag == FUSB_EHCI_ADD_HCD){
		fusb_ehci_add_hcd_flag = FUSB_EHCI_REMOVE_HCD;
		spin_unlock_irqrestore(&ehci->lock, flags);
		usb_remove_hcd(hcd);
		spin_lock_irqsave(&ehci->lock, flags);
	}
	spin_unlock_irqrestore(&ehci->lock, flags);

	/* release device resource */
	platform_set_drvdata(pdev, NULL);
	usb_put_hcd(hcd);

	dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");
	return 0;
}

/* So far, it is implementation of F_USB20HO device driver probing / removing */
/************************************************************************************************/

/* F_USB20HO device driver structure */
/**
 * @brief ehci_hcd_f_usb20ho_driver
 *
 *
 */
static struct platform_driver ehci_hcd_f_usb20ho_driver = {
	.probe = ehci_hcd_f_usb20ho_probe,
	.remove = ehci_hcd_f_usb20ho_remove,
	.shutdown = usb_hcd_platform_shutdown,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		.name = "f_usb20ho_ehci",
		.bus = &platform_bus_type
	}
};
/* F_USB20HO device module definition */
MODULE_ALIAS("platform:f_usb20ho_ehci");	/**<  */

