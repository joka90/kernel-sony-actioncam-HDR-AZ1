/*
 * drivers/usb/host/ohci-f_usb20ho.c
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
#include "linux/usb/f_usb/usb_otg_control.h"

//#define FUSB_FUNC_TRACE

/* import usb disabled check function */
extern int usb_disabled(void);          /**<  */

/**
 * @brief is_enable_ohci_irq
 *
 *
*/
static volatile bool is_enable_ohci_irq = true;

/**
 * @brief ohci_otg_ops
 *
 *
*/
struct usb_otg_module_ops ohci_otg_ops;

#define FUSB_OHCI_REMOVE_HCD	0
#define FUSB_OHCI_ADD_HCD	1U

/**
 * @brief fusb_ehci_add_hcd_flag
 *
 *
*/
static unsigned int fusb_ohci_add_hcd_flag = FUSB_OHCI_REMOVE_HCD;

#ifdef FUSB_FUNC_TRACE
#define fusb_func_trace(b)		printk("CALL %s %s(%d)\n",(b),__FUNCTION__,__LINE__)
#else
#define fusb_func_trace(b)
#endif

/* So far, it is implementation of F_USB20HO device driver local function */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HO device driver upper layer interface */

/*----------------------------------------------------------------------------*/
/* Start an OHCI controller, set the BUS operational
 * resets USB and controller
 * enable interrupts
 */
 /*
 * implementation: initial ohci status
 *      operational -> reset
 */
static int ohci_f_usb20ho_run (struct ohci_hcd *ohci)
{
	u32			mask, val;
	int			first = ohci->fminterval == 0;
	struct usb_hcd		*hcd = ohci_to_hcd(ohci);

	disable (ohci);

	/* boot firmware should have set this up (5.1.1.3.1) */
	if (first) {

		val = ohci_readl (ohci, &ohci->regs->fminterval);
		ohci->fminterval = val & 0x3fff;
		if (ohci->fminterval != FI)
			ohci_dbg (ohci, "fminterval delta %d\n",
				ohci->fminterval - FI);
		ohci->fminterval |= FSMP (ohci->fminterval) << 16;
		/* also: power/overcurrent flags in roothub.a */
	}

	/* Reset USB nearly "by the book".  RemoteWakeupConnected has
	 * to be checked in case boot firmware (BIOS/SMM/...) has set up
	 * wakeup in a way the bus isn't aware of (e.g., legacy PCI PM).
	 * If the bus glue detected wakeup capability then it should
	 * already be enabled; if so we'll just enable it again.
	 */
	if ((ohci->hc_control & OHCI_CTRL_RWC) != 0)
		device_set_wakeup_capable(hcd->self.controller, 1);

	switch (ohci->hc_control & OHCI_CTRL_HCFS) {
	case OHCI_USB_OPER:
		val = 0;
		break;
	case OHCI_USB_SUSPEND:
	case OHCI_USB_RESUME:
		ohci->hc_control &= OHCI_CTRL_RWC;
		ohci->hc_control |= OHCI_USB_RESUME;
		val = 10 /* msec wait */;
		break;
	// case OHCI_USB_RESET:
	default:
		ohci->hc_control &= OHCI_CTRL_RWC;
		ohci->hc_control |= OHCI_USB_RESET;
		val = 50 /* msec wait */;
		break;
	}
	ohci_writel (ohci, ohci->hc_control, &ohci->regs->control);
	// flush the writes
	(void) ohci_readl (ohci, &ohci->regs->control);
	msleep(val);

	memset (ohci->hcca, 0, sizeof (struct ohci_hcca));

	/* 2msec timelimit here means no irqs/preempt */
	spin_lock_irq (&ohci->lock);

retry:
	/* HC Reset requires max 10 us delay */
	ohci_writel (ohci, OHCI_HCR,  &ohci->regs->cmdstatus);
	val = 30;	/* ... allow extra time */
	while ((ohci_readl (ohci, &ohci->regs->cmdstatus) & OHCI_HCR) != 0) {
		if (--val == 0) {
			spin_unlock_irq (&ohci->lock);
			ohci_err (ohci, "USB HC reset timed out!\n");
			return -1;
		}
		udelay (1);
	}

	/* now we're in the SUSPEND state ... must go OPERATIONAL
	 * within 2msec else HC enters RESUME
	 *
	 * ... but some hardware won't init fmInterval "by the book"
	 * (SiS, OPTi ...), so reset again instead.  SiS doesn't need
	 * this if we write fmInterval after we're OPERATIONAL.
	 * Unclear about ALi, ServerWorks, and others ... this could
	 * easily be a longstanding bug in chip init on Linux.
	 */
	if (ohci->flags & OHCI_QUIRK_INITRESET) {
		ohci_writel (ohci, ohci->hc_control, &ohci->regs->control);
		// flush those writes
		(void) ohci_readl (ohci, &ohci->regs->control);
	}

	/* Tell the controller where the control and bulk lists are
	 * The lists are empty now. */
	ohci_writel (ohci, 0, &ohci->regs->ed_controlhead);
	ohci_writel (ohci, 0, &ohci->regs->ed_bulkhead);

	/* a reset clears this */
	ohci_writel (ohci, (u32) ohci->hcca_dma, &ohci->regs->hcca);

	periodic_reinit (ohci);

	/* some OHCI implementations are finicky about how they init.
	 * bogus values here mean not even enumeration could work.
	 */
	if ((ohci_readl (ohci, &ohci->regs->fminterval) & 0x3fff0000) == 0
			|| !ohci_readl (ohci, &ohci->regs->periodicstart)) {
		if (!(ohci->flags & OHCI_QUIRK_INITRESET)) {
			ohci->flags |= OHCI_QUIRK_INITRESET;
			ohci_dbg (ohci, "enabling initreset quirk\n");
			goto retry;
		}
		spin_unlock_irq (&ohci->lock);
		ohci_err (ohci, "init err (%08x %04x)\n",
			ohci_readl (ohci, &ohci->regs->fminterval),
			ohci_readl (ohci, &ohci->regs->periodicstart));
		return -EOVERFLOW;
	}

	/* use rhsc irqs after khubd is fully initialized */
	set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	hcd->uses_new_polling = 1;

	/* start controller operations */
	ohci->hc_control &= OHCI_CTRL_RWC;
#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	/*
	 * [notice]
	 * OHCI cannot be started when EHCI has started.
	 */
	ohci->hc_control |= OHCI_CONTROL_INIT | OHCI_USB_SUSPEND;
	ohci_writel (ohci, ohci->hc_control, &ohci->regs->control);
	hcd->state = HC_STATE_SUSPENDED;
	ohci->autostop = 1;
	fusb_otg_host_enable_irq(FUSB_OTG_HOST_EHCI, true);
#else
	ohci->hc_control |= OHCI_CONTROL_INIT | OHCI_USB_OPER;
	ohci_writel (ohci, ohci->hc_control, &ohci->regs->control);
	hcd->state = HC_STATE_RUNNING;
#endif

	/* wake on ConnectStatusChange, matching external hubs */
	ohci_writel (ohci, RH_HS_DRWE, &ohci->regs->roothub.status);

	/* Choose the interrupts we care about now, others later on demand */
	mask = OHCI_INTR_INIT;
	ohci_writel (ohci, ~0, &ohci->regs->intrstatus);
	ohci_writel (ohci, mask, &ohci->regs->intrenable);

	/* handle root hub init quirks ... */
	val = roothub_a (ohci);
	val &= ~(RH_A_PSM | RH_A_OCPM);
	if (ohci->flags & OHCI_QUIRK_SUPERIO) {
		/* NSC 87560 and maybe others */
		val |= RH_A_NOCP;
		val &= ~(RH_A_POTPGT | RH_A_NPS);
		ohci_writel (ohci, val, &ohci->regs->roothub.a);
	} else if ((ohci->flags & OHCI_QUIRK_AMD756) ||
			(ohci->flags & OHCI_QUIRK_HUB_POWER)) {
		/* hub power always on; required for AMD-756 and some
		 * Mac platforms.  ganged overcurrent reporting, if any.
		 */
		val |= RH_A_NPS;
		ohci_writel (ohci, val, &ohci->regs->roothub.a);
	}
	ohci_writel (ohci, RH_HS_LPSC, &ohci->regs->roothub.status);
	ohci_writel (ohci, (val & RH_A_NPS) ? 0 : RH_B_PPCM,
						&ohci->regs->roothub.b);
	// flush those writes
	(void) ohci_readl (ohci, &ohci->regs->control);

	ohci->next_statechange = jiffies + STATECHANGE_DELAY;
	spin_unlock_irq (&ohci->lock);

	// POTPGT delay is bits 24-31, in 2 ms units.
	mdelay ((val >> 23) & 0x1fe);
	hcd->state = HC_STATE_RUNNING;

	if (quirk_zfmicro(ohci)) {
		/* Create timer to watch for bad queue state on ZF Micro */
		setup_timer(&ohci->unlink_watchdog, unlink_watchdog_func,
				(unsigned long) ohci);

		ohci->eds_scheduled = 0;
		ohci->ed_to_check = NULL;
	}

	ohci_dump (ohci, 1);

	return 0;
}

#if defined(CONFIG_PM) || defined(CONFIG_PCI)
/*----------------------------------------------------------------------------*/
/* must not be called from interrupt context */
/*
 * implementation:
 *    ohci_run -> ohci_f_usb20ho_run
 */
static int ohci_f_usb20ho_restart (struct ohci_hcd *ohci)
{
	int temp;
	int i;
	struct urb_priv *priv;

	spin_lock_irq(&ohci->lock);
	disable (ohci);

	/* Recycle any "live" eds/tds (and urbs). */
	if (!list_empty (&ohci->pending))
		ohci_dbg(ohci, "abort schedule...\n");
	list_for_each_entry (priv, &ohci->pending, pending) {
		struct urb	*urb = priv->td[0]->urb;
		struct ed	*ed = priv->ed;

		switch (ed->state) {
		case ED_OPER:
			ed->state = ED_UNLINK;
			ed->hwINFO |= cpu_to_hc32(ohci, ED_DEQUEUE);
			ed_deschedule (ohci, ed);

			ed->ed_next = ohci->ed_rm_list;
			ed->ed_prev = NULL;
			ohci->ed_rm_list = ed;
			/* FALLTHROUGH */
		case ED_UNLINK:
			break;
		default:
			ohci_dbg(ohci, "bogus ed %p state %d\n",
					ed, ed->state);
		}

		if (!urb->unlinked)
			urb->unlinked = -ESHUTDOWN;
	}
	finish_unlinks (ohci, 0);
	spin_unlock_irq(&ohci->lock);

	/* paranoia, in case that didn't work: */

	/* empty the interrupt branches */
	for (i = 0; i < NUM_INTS; i++) ohci->load [i] = 0;
	for (i = 0; i < NUM_INTS; i++) ohci->hcca->int_table [i] = 0;

	/* no EDs to remove */
	ohci->ed_rm_list = NULL;

	/* empty control and bulk lists */
	ohci->ed_controltail = NULL;
	ohci->ed_bulktail    = NULL;

	if ((temp = ohci_f_usb20ho_run (ohci)) < 0) {
		ohci_err (ohci, "can't restart, %d\n", temp);
		return temp;
	}
	ohci_dbg(ohci, "restart complete\n");
	return 0;
}
#endif

#ifdef	CONFIG_PM
/*----------------------------------------------------------------------------*/
/* caller has locked the root hub */
/*
 * implementation:
 *    ohci_restart -> ohci_f_usb20ho_restart
 */
static int ohci_f_usb20ho_rh_resume (struct ohci_hcd *ohci)
__releases(ohci->lock)
__acquires(ohci->lock)
{
	struct usb_hcd		*hcd = ohci_to_hcd (ohci);
	u32			temp, enables;
	int			status = -EINPROGRESS;
	int			autostopped = ohci->autostop;

	ohci->autostop = 0;
	ohci->hc_control = ohci_readl (ohci, &ohci->regs->control);

	if (ohci->hc_control & (OHCI_CTRL_IR | OHCI_SCHED_ENABLES)) {
		/* this can happen after resuming a swsusp snapshot */
		if (hcd->state == HC_STATE_RESUMING) {
			ohci_dbg (ohci, "BIOS/SMM active, control %03x\n",
					ohci->hc_control);
			status = -EBUSY;
		/* this happens when pmcore resumes HC then root */
		} else {
			ohci_dbg (ohci, "duplicate resume\n");
			status = 0;
		}
	} else switch (ohci->hc_control & OHCI_CTRL_HCFS) {
	case OHCI_USB_SUSPEND:
		ohci->hc_control &= ~(OHCI_CTRL_HCFS|OHCI_SCHED_ENABLES);
		ohci->hc_control |= OHCI_USB_RESUME;
		ohci_writel (ohci, ohci->hc_control, &ohci->regs->control);
		(void) ohci_readl (ohci, &ohci->regs->control);
		ohci_dbg (ohci, "%s root hub\n",
				autostopped ? "auto-start" : "resume");
		break;
	case OHCI_USB_RESUME:
		/* HCFS changes sometime after INTR_RD */
		ohci_dbg(ohci, "%swakeup root hub\n",
				autostopped ? "auto-" : "");
		break;
	case OHCI_USB_OPER:
		/* this can happen after resuming a swsusp snapshot */
		ohci_dbg (ohci, "snapshot resume? reinit\n");
		status = -EBUSY;
		break;
	default:		/* RESET, we lost power */
		ohci_dbg (ohci, "lost power\n");
		status = -EBUSY;
	}
	if (status == -EBUSY) {
		if (!autostopped) {
			spin_unlock_irq (&ohci->lock);
			(void) ohci_init (ohci);
			status = ohci_f_usb20ho_restart (ohci);

			usb_root_hub_lost_power(hcd->self.root_hub);

			spin_lock_irq (&ohci->lock);
		}
		return status;
	}
	if (status != -EINPROGRESS)
		return status;
	if (autostopped)
		goto skip_resume;
	spin_unlock_irq (&ohci->lock);

	/* Some controllers (lucent erratum) need extra-long delays */
	msleep (20 /* usb 11.5.1.10 */ + 12 /* 32 msec counter */ + 1);

	temp = ohci_readl (ohci, &ohci->regs->control);
	temp &= OHCI_CTRL_HCFS;
	if (temp != OHCI_USB_RESUME) {
		ohci_err (ohci, "controller won't resume\n");
		spin_lock_irq(&ohci->lock);
		return -EBUSY;
	}

	/* disable old schedule state, reinit from scratch */
	ohci_writel (ohci, 0, &ohci->regs->ed_controlhead);
	ohci_writel (ohci, 0, &ohci->regs->ed_controlcurrent);
	ohci_writel (ohci, 0, &ohci->regs->ed_bulkhead);
	ohci_writel (ohci, 0, &ohci->regs->ed_bulkcurrent);
	ohci_writel (ohci, 0, &ohci->regs->ed_periodcurrent);
	ohci_writel (ohci, (u32) ohci->hcca_dma, &ohci->regs->hcca);

	/* Sometimes PCI D3 suspend trashes frame timings ... */
	periodic_reinit (ohci);

	/* the following code is executed with ohci->lock held and
	 * irqs disabled if and only if autostopped is true
	 */

skip_resume:
	/* interrupts might have been disabled */
	ohci_writel (ohci, OHCI_INTR_INIT, &ohci->regs->intrenable);
	if (ohci->ed_rm_list)
		ohci_writel (ohci, OHCI_INTR_SF, &ohci->regs->intrenable);

	/*
	 * [notice]
	 * OHCI cannot be started when EHCI has started.
	 */
#ifndef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	/* Then re-enable operations */
	ohci_writel (ohci, OHCI_USB_OPER, &ohci->regs->control);
#endif
	(void) ohci_readl (ohci, &ohci->regs->control);
	if (!autostopped)
		msleep (3);

	temp = ohci->hc_control;
	temp &= OHCI_CTRL_RWC;

	/*
	 * [notice]
	 * OHCI cannot be started when EHCI has started.
	 */
#ifndef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	temp |= OHCI_CONTROL_INIT | OHCI_USB_OPER;
	ohci->hc_control = temp;
	ohci_writel (ohci, temp, &ohci->regs->control);
#endif
	(void) ohci_readl (ohci, &ohci->regs->control);

	/* TRSMRCY */
	if (!autostopped) {
		msleep (10);
		spin_lock_irq (&ohci->lock);
	}
	/* now ohci->lock is always held and irqs are always disabled */

	/* keep it alive for more than ~5x suspend + resume costs */
	ohci->next_statechange = jiffies + STATECHANGE_DELAY;

	/* maybe turn schedules back on */
	enables = 0;
	temp = 0;
	if (!ohci->ed_rm_list) {
		if (ohci->ed_controltail) {
			ohci_writel (ohci,
					find_head (ohci->ed_controltail)->dma,
					&ohci->regs->ed_controlhead);
			enables |= OHCI_CTRL_CLE;
			temp |= OHCI_CLF;
		}
		if (ohci->ed_bulktail) {
			ohci_writel (ohci, find_head (ohci->ed_bulktail)->dma,
				&ohci->regs->ed_bulkhead);
			enables |= OHCI_CTRL_BLE;
			temp |= OHCI_BLF;
		}
	}
	if (hcd->self.bandwidth_isoc_reqs || hcd->self.bandwidth_int_reqs)
		enables |= OHCI_CTRL_PLE|OHCI_CTRL_IE;
	if (enables) {
		ohci_dbg (ohci, "restarting schedules ... %08x\n", enables);
		ohci->hc_control |= enables;
		ohci_writel (ohci, ohci->hc_control, &ohci->regs->control);
		if (temp)
			ohci_writel (ohci, temp, &ohci->regs->cmdstatus);
		(void) ohci_readl (ohci, &ohci->regs->control);
	}

	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_f_usb20ho_irq
 *
 *
 *
 * @param [in]  hcd
 *
 * @return
 * @retval IRQ_HANDLED
 * @retval IRQ_NOTMINE
 */
/*----------------------------------------------------------------------------*/
/* an interrupt happens */
static irqreturn_t ohci_f_usb20ho_irq (struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci (hcd);
	struct ohci_regs __iomem *regs = ohci->regs;
	int ints, i;

	fusb_func_trace("START");

	fvc2_h2xbb_complete_wait();


	/* Read interrupt status (and flush pending writes).  We ignore the
	 * optimization of checking the LSB of hcca->done_head; it doesn't
	 * work on all systems (edge triggering for OHCI can be a factor).
	 */
	ints = ohci_readl(ohci, &regs->intrstatus);

	/* Check for an all 1's result which is a typical consequence
	 * of dead, unclocked, or unplugged (CardBus...) devices
	 */
	if (ints == ~(u32)0) {
		disable (ohci);
		ohci_dbg (ohci, "device removed!\n");
		usb_hc_died(hcd);
		return IRQ_HANDLED;
	}

	/* We only care about interrupts that are enabled */
	ints &= ohci_readl(ohci, &regs->intrenable);

	/* interrupt for some other device? */
	if (ints == 0 || unlikely(hcd->state == HC_STATE_HALT))
		return IRQ_NOTMINE;

	if (ints & OHCI_INTR_UE) {
		// e.g. due to PCI Master/Target Abort
		if (quirk_nec(ohci)) {
			/* Workaround for a silicon bug in some NEC chips used
			 * in Apple's PowerBooks. Adapted from Darwin code.
			 */
			ohci_err (ohci, "OHCI Unrecoverable Error, scheduling NEC chip restart\n");

			ohci_writel (ohci, OHCI_INTR_UE, &regs->intrdisable);

			schedule_work (&ohci->nec_work);
		} else {
			disable (ohci);
			ohci_err (ohci, "OHCI Unrecoverable Error, disabled\n");
			usb_hc_died(hcd);
		}

		ohci_dump (ohci, 1);
		ohci_usb_reset (ohci);
	}

	if (ints & OHCI_INTR_RHSC) {
		ohci_vdbg(ohci, "rhsc\n");
		ohci->next_statechange = jiffies + STATECHANGE_DELAY;
		ohci_writel(ohci, OHCI_INTR_RD | OHCI_INTR_RHSC,
				&regs->intrstatus);

#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
		/* look at each port */
		for (i = 0; i < ohci->num_ports; i++) {
			u32 status = roothub_portstatus (ohci, i);
			u32 temp;
			if((status & RH_PS_CCS) && (status & RH_PS_CSC)) {
				/*
				 * [notice]
				 * When a full speed device is connected
				 * and port owner is changed to OHCI,
				 * it is necessary to start OHCI operation.
				 */
				ohci_dbg (ohci, "[CONFLICT_GUARD]ohci start.\n");
				fusb_otg_host_enable_irq(FUSB_OTG_HOST_EHCI, false);
				temp = ohci->hc_control;
				temp &= ~OHCI_CTRL_HCFS;
				temp |= OHCI_USB_OPER;
				ohci_writel (ohci, temp, &ohci->regs->control);
				ohci->hc_control = temp;
				while((ohci_readl(ohci, &ohci->regs->control) & OHCI_CTRL_HCFS) != OHCI_USB_OPER);
				hcd->state = HC_STATE_RUNNING;
				ohci_dbg (ohci, "[CONFLICT_GUARD]ohci start completed.\n");
				break;
			}
		}
#endif


		/* NOTE: Vendors didn't always make the same implementation
		 * choices for RHSC.  Many followed the spec; RHSC triggers
		 * on an edge, like setting and maybe clearing a port status
		 * change bit.  With others it's level-triggered, active
		 * until khubd clears all the port status change bits.  We'll
		 * always disable it here and rely on polling until khubd
		 * re-enables it.
		 */
		ohci_writel(ohci, OHCI_INTR_RHSC, &regs->intrdisable);
		usb_hcd_poll_rh_status(hcd);
	}

	/* For connect and disconnect events, we expect the controller
	 * to turn on RHSC along with RD.  But for remote wakeup events
	 * this might not happen.
	 */
	else if (ints & OHCI_INTR_RD) {
		ohci_vdbg(ohci, "resume detect\n");
		ohci_writel(ohci, OHCI_INTR_RD, &regs->intrstatus);
		set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
		if (ohci->autostop) {
			spin_lock (&ohci->lock);
			ohci_f_usb20ho_rh_resume (ohci);
			spin_unlock (&ohci->lock);
		} else
			usb_hcd_resume_root_hub(hcd);
	}

	if (ints & OHCI_INTR_WDH) {
		if (ohci->hcca->done_head == 0) {
			ints &= ~OHCI_INTR_WDH;
		} else {
			spin_lock (&ohci->lock);
			dl_done_list (ohci);
			spin_unlock (&ohci->lock);
		}
	}

	if (quirk_zfmicro(ohci) && (ints & OHCI_INTR_SF)) {
		spin_lock(&ohci->lock);
		if (ohci->ed_to_check) {
			struct ed *ed = ohci->ed_to_check;

			if (check_ed(ohci, ed)) {
				/* HC thinks the TD list is empty; HCD knows
				 * at least one TD is outstanding
				 */
				if (--ohci->zf_delay == 0) {
					struct td *td = list_entry(
						ed->td_list.next,
						struct td, td_list);
					ohci_warn(ohci,
						  "Reclaiming orphan TD %p\n",
						  td);
					takeback_td(ohci, td);
					ohci->ed_to_check = NULL;
				}
			} else
				ohci->ed_to_check = NULL;
		}
		spin_unlock(&ohci->lock);
	}

	/* could track INTR_SO to reduce available PCI/... bandwidth */

	/* handle any pending URB/ED unlinks, leaving INTR_SF enabled
	 * when there's still unlinking to be done (next frame).
	 */
	spin_lock (&ohci->lock);
	if (ohci->ed_rm_list)
		finish_unlinks (ohci, ohci_frame_no(ohci));
	if ((ints & OHCI_INTR_SF) != 0
			&& !ohci->ed_rm_list
			&& !ohci->ed_to_check
			&& HC_IS_RUNNING(hcd->state))
		ohci_writel (ohci, OHCI_INTR_SF, &regs->intrdisable);
	spin_unlock (&ohci->lock);

	if (HC_IS_RUNNING(hcd->state)) {
		ohci_writel (ohci, ints, &regs->intrstatus);
		ohci_writel (ohci, OHCI_INTR_MIE, &regs->intrenable);
		// flush those writes
		(void) ohci_readl (ohci, &ohci->regs->control);
	}

	fusb_func_trace("END");

	return IRQ_HANDLED;
}

static int ohci_f_usb20ho_reset(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int result;

	fusb_func_trace("START");

	ohci_dbg(ohci, "%s() is started.\n", __FUNCTION__);

	/* initialize ohci */
	result = ohci_init(ohci);
	if (result) {
		ohci_err(ohci, "ohci_init() is failed at %d.", result);
		ohci_dbg(ohci, "%s() is ended.\n", __FUNCTION__);
		return result;
	}

	ohci_dbg(ohci, "%s() is ended.\n", __FUNCTION__);

#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	fusb_otg_host_enable_irq(FUSB_OTG_HOST_EHCI, true);
#endif


	return 0;
}

static int ohci_f_usb20ho_start(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int result;

	ohci_dbg(ohci, "%s() is started.\n", __FUNCTION__);

	/* enable bus power supply */
	enable_bus_power_supply(hcd->regs, 1U);

	/* run ohci */
	result = ohci_f_usb20ho_run(ohci);
	if (result) {
		ohci_err(ohci, "ohci_f_usb20ho_run() is failed at %d.", result);
		enable_bus_power_supply(hcd->regs, 0);
		ohci_stop(hcd);
		ohci_dbg(ohci, "%s() is ended.\n", __FUNCTION__);
		return result;
	}

	ohci_dbg(ohci, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");
	return 0;
}


/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_f_usb20ho_finish_controller_resume
 *
 * Carry out the final steps of resuming the controller device
 *
 * @param [in]  hcd
 *
 * @return
 */
/*----------------------------------------------------------------------------*/
static void ohci_f_usb20ho_finish_controller_resume(struct usb_hcd *hcd)
{
	struct ohci_hcd		*ohci = hcd_to_ohci(hcd);
	int			port;
	bool			need_reinit = false;

	/* See if the controller is already running or has been reset */
	ohci->hc_control = ohci_readl(ohci, &ohci->regs->control);
	if (ohci->hc_control & (OHCI_CTRL_IR | OHCI_SCHED_ENABLES)) {
		ohci_dbg(ohci, "ohci_finish_controller_resume:%d\n", __LINE__);
		need_reinit = true;
	} else {
		switch (ohci->hc_control & OHCI_CTRL_HCFS) {
		case OHCI_USB_OPER:
		case OHCI_USB_RESET:
			need_reinit = true;
		}
		ohci_dbg(ohci, "ohci_finish_controller_resume:%d, hccontrol:%x\n", __LINE__, ohci->hc_control);
	}

	/* If needed, reinitialize and suspend the root hub */
	if (need_reinit) {
		spin_lock_irq(&ohci->lock);
		hcd->state = HC_STATE_RESUMING;
		ohci_f_usb20ho_rh_resume(ohci);
		hcd->state = HC_STATE_QUIESCING;
		ohci_rh_suspend(ohci, 0);
		hcd->state = HC_STATE_SUSPENDED;
		spin_unlock_irq(&ohci->lock);
		ohci_dbg(ohci, "ohci_finish_controller_resume:%d\n", __LINE__);
	}

	/* Normally just turn on port power and enable interrupts */
	else {
		ohci_dbg(ohci, "powerup ports\n");
		for (port = 0; port < ohci->num_ports; port++)
			ohci_writel(ohci, RH_PS_PPS,
					&ohci->regs->roothub.portstatus[port]);

		ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrenable);
		ohci_readl(ohci, &ohci->regs->intrenable);
		msleep(20);
		ohci_dbg(ohci, "ohci_finish_controller_resume:%d\n", __LINE__);
	}

	usb_hcd_resume_root_hub(hcd);
}

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_f_usb20ho_pci_suspend
 *
 *
 *
 * @param [in]  hcd
 * @param [in]  do_wakeup
 *
 * @return
 * @retval 0
 * @retval -EINVAL
 */
/*----------------------------------------------------------------------------*/
static int ohci_f_usb20ho_pci_suspend(struct usb_hcd *hcd, bool do_wakeup)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	unsigned long	flags;
	int		rc = 0;

	fusb_func_trace("START");

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave (&ohci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrdisable);
	(void)ohci_readl(ohci, &ohci->regs->intrdisable);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
 bail:
	spin_unlock_irqrestore (&ohci->lock, flags);

	fusb_func_trace("END");
	return rc;
}

static void ohci_f_usb20ho_enable_irq (bool isEnable)
{
	fusb_func_trace("START");

	if (isEnable) {
		if (!is_enable_ohci_irq)
			enable_irq(F_USB_HO_OHCI_IRQ);
	} else {
		if (is_enable_ohci_irq)
			disable_irq(F_USB_HO_OHCI_IRQ);
	}

	is_enable_ohci_irq = isEnable;

	fusb_func_trace("END");
}

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_f_usb20ho_pci_resume
 *
 *
 *
 * @param [in]  hcd
 * @param [in]  hibernated
 *
 * @return
 * @retval 0
 */
/*----------------------------------------------------------------------------*/
static int ohci_f_usb20ho_pci_resume(struct usb_hcd *hcd, bool hibernated)
{
	fusb_func_trace("START");

#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	ohci_f_usb20ho_enable_irq(false);
#endif

	fusb_otg_resume_initial_set();

	ohci_f_usb20ho_reset(hcd);

	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	ohci_usb_reset(hcd_to_ohci(hcd));

	ohci_f_usb20ho_finish_controller_resume(hcd);

	fusb_func_trace("END");
	return 0;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_f_usb20ho_bus_resume
 *
 *
 *
 * @param [in]  hcd
 *
 * @return rc
 */
/*----------------------------------------------------------------------------*/
static int ohci_f_usb20ho_bus_resume (struct usb_hcd *hcd)
{
	struct ohci_hcd		*ohci = hcd_to_ohci (hcd);
	int			rc;

	if (time_before (jiffies, ohci->next_statechange))
		msleep(5);

	spin_lock_irq (&ohci->lock);

	if (unlikely(!HCD_HW_ACCESSIBLE(hcd)))
		rc = -ESHUTDOWN;
	else
		rc = ohci_f_usb20ho_rh_resume (ohci);
	spin_unlock_irq (&ohci->lock);

	/* poll until we know a device is connected or we autostop */
	if (rc == 0)
		usb_hcd_poll_rh_status(hcd);

#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	fusb_otg_host_resume_finish(FUSB_OTG_HOST_OHCI);
#endif

	return rc;
}

static void ohci_f_usb20ho_stop(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci;

	fusb_func_trace("START");

	ohci = hcd_to_ohci(hcd);

	ohci_dbg(ohci, "%s() is started.\n", __FUNCTION__);

	/* disable bus power supply */
	enable_bus_power_supply(hcd->regs, 0);

	/* stop ohci */
	ohci_stop(hcd);

	ohci_dbg(ohci, "%s() is ended.\n", __FUNCTION__);

#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	fusb_otg_host_enable_irq(FUSB_OTG_HOST_EHCI, true);
#endif

	fusb_func_trace("END");

	return;
}

#ifdef	CONFIG_PM
/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_f_usb20ho_root_hub_state_changes
 *
 *
 *
 * @param [in]  ohci
 * @param [in]  changed
 * @param [in]  any_connected
 * @param [in]  rhsc_status
 *
 * @return
 * @retval poll_rh
 */
/*----------------------------------------------------------------------------*/
/* Carry out polling-, autostop-, and autoresume-related state changes */
static int ohci_f_usb20ho_root_hub_state_changes(struct ohci_hcd *ohci, int changed,
		int any_connected, int rhsc_status)
{
	int	poll_rh = 1;
	int	rhsc_enable;

	/* Some broken controllers never turn off RHSC in the interrupt
	 * status register.  For their sake we won't re-enable RHSC
	 * interrupts if the interrupt bit is already active.
	 */
	rhsc_enable = ohci_readl(ohci, &ohci->regs->intrenable) &
			OHCI_INTR_RHSC;

	switch (ohci->hc_control & OHCI_CTRL_HCFS) {
	case OHCI_USB_OPER:
		/* If no status changes are pending, enable RHSC interrupts. */
		if (!rhsc_enable && !rhsc_status && !changed) {
			rhsc_enable = OHCI_INTR_RHSC;
			ohci_writel(ohci, rhsc_enable, &ohci->regs->intrenable);
		}

		/* Keep on polling until we know a device is connected
		 * and RHSC is enabled, or until we autostop.
		 */
		if (!ohci->autostop) {
			if (any_connected ||
					!device_may_wakeup(&ohci_to_hcd(ohci)
						->self.root_hub->dev)) {
				if (rhsc_enable)
					poll_rh = 0;
			} else {
				ohci->autostop = 1;
				ohci->next_statechange = jiffies + HZ;
			}

		/* if no devices have been attached for one second, autostop */
		} else {
			if (changed || any_connected) {
				ohci->autostop = 0;
				ohci->next_statechange = jiffies +
						STATECHANGE_DELAY;
			} else if (time_after_eq(jiffies,
						ohci->next_statechange)
					&& !ohci->ed_rm_list
					&& !(ohci->hc_control &
						OHCI_SCHED_ENABLES)) {
				ohci_rh_suspend(ohci, 1);
#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
				fusb_otg_host_enable_irq(FUSB_OTG_HOST_EHCI, true);
#endif
				if (rhsc_enable)
					poll_rh = 0;
			}
		}
		break;

	case OHCI_USB_RESUME:
#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
		/* If no status changes are pending, enable RHSC interrupts. */
		if (!rhsc_enable && !rhsc_status && !changed) {
			rhsc_enable = OHCI_INTR_RHSC;
			ohci_writel(ohci, rhsc_enable, &ohci->regs->intrenable);
		}
#endif
	case OHCI_USB_SUSPEND:
		/* if there is a port change, autostart or ask to be resumed */
		if (changed) {
			if (ohci->autostop)
				ohci_f_usb20ho_rh_resume(ohci);
			else
				usb_hcd_resume_root_hub(ohci_to_hcd(ohci));

		/* If remote wakeup is disabled, stop polling */
		} else if (!ohci->autostop &&
				!ohci_to_hcd(ohci)->self.root_hub->
					do_remote_wakeup) {
			poll_rh = 0;

		} else {
			/* If no status changes are pending,
			 * enable RHSC interrupts
			 */
			if (!rhsc_enable && !rhsc_status) {
				rhsc_enable = OHCI_INTR_RHSC;
				ohci_writel(ohci, rhsc_enable,
						&ohci->regs->intrenable);
			}
			/* Keep polling until RHSC is enabled */
			if (rhsc_enable)
				poll_rh = 0;
		}
		break;
	}
	return poll_rh;
}
#endif

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_f_usb20ho_hub_status_data
 *
 *
 *
 * @param [in]  hcd
 * @param [in]  buf
 *
 * @return
 * @retval 0
 * @retval 1
 * @retval -EINVA
 */
/*----------------------------------------------------------------------------*/
static int ohci_f_usb20ho_hub_status_data( struct usb_hcd *hcd , char *buf )
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		i, changed = 0, length = 1;
	int		any_connected = 0;
	int		rhsc_status;
	unsigned long	flags;

	fusb_func_trace("START");

	if((hcd == NULL ) || (buf == NULL)) {
		return -EINVAL;
	}

	spin_lock_irqsave (&ohci->lock, flags);
	if (!HCD_HW_ACCESSIBLE(hcd))
		goto done;

	/* undocumented erratum seen on at least rev D */
	if ((ohci->flags & OHCI_QUIRK_AMD756)
			&& (roothub_a (ohci) & RH_A_NDP) > MAX_ROOT_PORTS) {
		ohci_warn (ohci, "bogus NDP, rereads as NDP=%d\n",
			  ohci_readl (ohci, &ohci->regs->roothub.a) & RH_A_NDP);
		/* retry later; "should not happen" */
		goto done;
	}

#ifdef CONFIG_USB_EHCI_F_USB20HO_USED_CONFLICT_GUARD
	if(!is_enable_ohci_irq)
		goto done;
#endif

	/* init status */
	if (roothub_status (ohci) & (RH_HS_LPSC | RH_HS_OCIC))
		buf [0] = changed = 1;
	else
		buf [0] = 0;
	if (ohci->num_ports > 7) {
		buf [1] = 0;
		length++;
	}

	/* Clear the RHSC status flag before reading the port statuses */
	ohci_writel(ohci, OHCI_INTR_RHSC, &ohci->regs->intrstatus);
	rhsc_status = ohci_readl(ohci, &ohci->regs->intrstatus) &
			OHCI_INTR_RHSC;

	/* look at each port */
	for (i = 0; i < ohci->num_ports; i++) {
		u32	status = roothub_portstatus (ohci, i);

		/* can't autostop if ports are connected */
		any_connected |= (status & RH_PS_CCS);

		if (status & (RH_PS_CSC | RH_PS_PESC | RH_PS_PSSC
				| RH_PS_OCIC | RH_PS_PRSC)) {
			changed = 1;
			if (i < 7)
			    buf [0] |= 1 << (i + 1);
			else
			    buf [1] |= 1 << (i - 7);
		}

		if( status & ( RH_PS_OCIC | RH_PS_POCI ) ){
			fusb_otg_notify_vbuserror();
		}

		if( status & RH_PS_CSC ){
			if( status & RH_PS_CCS )
				fusb_otg_notify_connect( FUSB_EVENT_CONNECT );
			else
				fusb_otg_notify_connect( FUSB_EVENT_DISCONNECT );
		}
	}

	if (ohci_f_usb20ho_root_hub_state_changes(ohci, changed,
			any_connected, rhsc_status))
		set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	else
		clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);

done:
	spin_unlock_irqrestore (&ohci->lock, flags);

	fusb_func_trace("END");

	return changed ? length : 0;
}

/* So far, it is implementation of F_USB20HO device driver upper layer interface */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HO device driver probing / removing */
void ohci_hcd_f_usb20ho_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	f_usb20ho_unmap_urb_for_dma(hcd, urb);
	
	return;
}

static int ohci_hcd_f_usb20ho_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
			    gfp_t mem_flags)
{
	int ret = 0;

	ret = f_usb20ho_map_urb_for_dma(hcd, urb, mem_flags);
	
	return ret;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_hcd_f_usb20ho_otg_probe
 *
 *
 *
 * @param [in]  data
 *
 * @return
 * @retval result
 */
/*----------------------------------------------------------------------------*/
static int ohci_hcd_f_usb20ho_otg_probe( void *data )
{
	int result = 0;
	int f_usb20ho_irq;
	struct usb_hcd *hcd = ( struct usb_hcd * )data;
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	unsigned long flags;

	fusb_func_trace("START");

	/* get an IRQ for a F_USB20HO device */
	f_usb20ho_irq = F_USB_HO_OHCI_IRQ;

	/* add hcd device */
	spin_lock_irqsave(&ohci->lock, flags);
	if( fusb_ohci_add_hcd_flag == FUSB_OHCI_REMOVE_HCD ) {
		spin_unlock_irqrestore(&ohci->lock, flags);
		result = usb_add_hcd(hcd, f_usb20ho_irq, 0);
		if (result) {
			enable_bus_power_supply(hcd, 0);
			usb_put_hcd(hcd);
			return result;
		}
		spin_lock_irqsave(&ohci->lock, flags);
		fusb_ohci_add_hcd_flag = FUSB_OHCI_ADD_HCD;
	}
	spin_unlock_irqrestore(&ohci->lock, flags);

	fusb_func_trace("END");

	return result;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_hcd_f_usb20ho_otg_remove
 *
 *
 *
 * @param [in]  data
 *
 * @return
 * @retval 0
 */
/*----------------------------------------------------------------------------*/
static int ohci_hcd_f_usb20ho_otg_remove( void *data )
{
	struct usb_hcd  *hcd = ( struct usb_hcd * )data;
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	unsigned long   flags;
	int             port;
	u32             val;

	fusb_func_trace("START");

	/* remove hcd device */
	spin_lock_irqsave(&ohci->lock, flags);
	if( fusb_ohci_add_hcd_flag == FUSB_OHCI_ADD_HCD ) {
		fusb_ohci_add_hcd_flag = FUSB_OHCI_REMOVE_HCD;

		spin_unlock_irqrestore(&ohci->lock, flags);
		usb_remove_hcd(hcd);
		spin_lock_irqsave(&ohci->lock, flags);
	}
	spin_unlock_irqrestore(&ohci->lock, flags);

	/* Clear NoPowerSwitching */
	val = ohci_readl (ohci, &ohci->regs->roothub.a);
	val &= ~(RH_A_NPS);
	ohci_writel (ohci, val, &ohci->regs->roothub.a);

	/* Clear Port Power */
	for (port = 0; port < ohci->num_ports; port++)
		ohci_writel(ohci, RH_PS_LSDA, &ohci->regs->roothub.portstatus[port] );

	fusb_func_trace("END");

	return 0;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_hcd_f_usb20ho_otg_resume
 *
 *
 *
 * @param [in]  data
 *
 * @return
 * @retval 0
 */
/*----------------------------------------------------------------------------*/
static int ohci_hcd_f_usb20ho_otg_resume( void *data )
{
	fusb_func_trace("START");

	fusb_func_trace("END");
	return 0;
}

static int ohci_hcd_f_usb20ho_probe(struct platform_device *pdev)
{
	static const struct hc_driver ohci_f_usb20ho_hc_driver = {
		.description = hcd_name,
		.product_desc = "F_USB20HO OHCI",
		.hcd_priv_size = sizeof(struct ohci_hcd),
		.irq = ohci_f_usb20ho_irq,
		.flags = HCD_MEMORY | HCD_USB11,
		.reset = ohci_f_usb20ho_reset,
		.start = ohci_f_usb20ho_start,
		.pci_suspend = ohci_f_usb20ho_pci_suspend ,
		.pci_resume = ohci_f_usb20ho_pci_resume ,
		.stop = ohci_f_usb20ho_stop,
		.shutdown = ohci_shutdown,
		.get_frame_number = ohci_get_frame,
		.urb_enqueue = ohci_urb_enqueue,
		.urb_dequeue = ohci_urb_dequeue,
		.map_urb_for_dma = ohci_hcd_f_usb20ho_map_urb_for_dma,
		.unmap_urb_for_dma = ohci_hcd_f_usb20ho_unmap_urb_for_dma,
		.endpoint_disable = ohci_endpoint_disable,
		.hub_status_data = ohci_f_usb20ho_hub_status_data,
		.hub_control = ohci_hub_control,
		.endpoint_disable = ohci_endpoint_disable,
#ifndef CONFIG_PM
		.bus_suspend = NULL,
		.bus_resume = NULL,
#else
		.bus_suspend = ohci_bus_suspend ,
		.bus_resume = ohci_f_usb20ho_bus_resume ,
#endif
		.start_port_reset = NULL ,
		.relinquish_port = NULL,
		.port_handed_over = NULL,
	};
	void __iomem *f_usb20ho_register_base_address;
	int f_usb20ho_irq;
	struct usb_hcd *hcd = NULL;
	struct ohci_hcd *ohci;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!pdev))
		return -EINVAL;

	dev_dbg(&pdev->dev, "%s() is started.\n", __FUNCTION__);

	/* driver version output */
	dev_dbg(&pdev->dev, "F_USB20HO OHCI driver version is %s.\n", F_USB20HO_CONFIG_OHCI_DRIVER_VERSION);

	/* check usb disabled */
	if (usb_disabled()) {
		dev_err(&pdev->dev, "usb_disabled() is true.\n");
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* create hcd */
	hcd = usb_create_hcd(&ohci_f_usb20ho_hc_driver, &pdev->dev, "f_usb20ho_ohci");
	if (!hcd) {
		dev_err(&pdev->dev, "usb_create_hcd() is failed.\n");
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENOMEM;
	}

	/* get a register base address for a F_USB20HO device */
	f_usb20ho_register_base_address = ( void __iomem * )fusb_otg_get_base_addr( FUSB_ADDR_TYPE_HOST );
	if (!f_usb20ho_register_base_address) {
		dev_err(&pdev->dev, "fusb_otg_get_base_addr() is failed.\n");
		usb_put_hcd(hcd);
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* get an IRQ for a F_USB20HO device */
	f_usb20ho_irq = F_USB_HO_OHCI_IRQ;

	/* setup the hcd structure parameter */
	hcd->regs = f_usb20ho_register_base_address + F_USB20HO_OHCI_BASE;

	g_fvc2_h2xbb_regs = ( void __iomem * )fusb_otg_get_base_addr( FUSB_ADDR_TYPE_H2XBB );

	if (!g_fvc2_h2xbb_regs) {
		dev_err(&pdev->dev, "fusb_otg_get_base_addr() is failed.\n");
		usb_put_hcd(hcd);
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* setup the ohci structure parameter */
	ohci = hcd_to_ohci(hcd);
	ohci->num_ports = 1;
	ohci_hcd_init(ohci);

	/* setup the private data of a device driver */
	platform_set_drvdata(pdev, hcd);

	ohci_otg_ops.pdev   = pdev;
	ohci_otg_ops.data   = ( void * )hcd;
	ohci_otg_ops.probe  = ohci_hcd_f_usb20ho_otg_probe;
	ohci_otg_ops.remove = ohci_hcd_f_usb20ho_otg_remove;
	ohci_otg_ops.resume = ohci_hcd_f_usb20ho_otg_resume;
	ohci_otg_ops.enable_irq = ohci_f_usb20ho_enable_irq;

	fusb_otg_host_ohci_bind( &ohci_otg_ops );

	fusb_func_trace("END");

	return 0;
}

/*----------------------------------------------------------------------------*/
/**
 * @brief ohci_hcd_f_usb20ho_remove
 *
 *
 *
 * @param [in]  pdev
 *
 * @return
 * @retval 0
 * @retval -EINVAL
 */
/*----------------------------------------------------------------------------*/
static int ohci_hcd_f_usb20ho_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct ohci_hcd *ohci;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!pdev))
		return -EINVAL;

	dev_dbg(&pdev->dev, "%s() is started.\n", __FUNCTION__);

	/* get hcd data */
	hcd = platform_get_drvdata(pdev);
	if (!hcd) {
		dev_err(&pdev->dev, "platform_get_drvdata() is failed.");
		dev_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* remove hcd device */
	ohci = hcd_to_ohci(hcd);

	spin_lock_irqsave(&ohci->lock, flags);
	if( fusb_ohci_add_hcd_flag == FUSB_OHCI_ADD_HCD ) {
		fusb_ohci_add_hcd_flag = FUSB_OHCI_REMOVE_HCD;

		spin_unlock_irqrestore(&ohci->lock, flags);
		usb_remove_hcd(hcd);
		spin_lock_irqsave(&ohci->lock, flags);
	}
	spin_unlock_irqrestore(&ohci->lock, flags);

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
 * @brief ohci_hcd_f_usb20ho_driver
 *
 *
 */
static struct platform_driver ohci_hcd_f_usb20ho_driver = {
	.probe = ohci_hcd_f_usb20ho_probe,
	.remove = ohci_hcd_f_usb20ho_remove,
	.shutdown = usb_hcd_platform_shutdown,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		.name = "f_usb20ho_ohci",
		.bus = &platform_bus_type
	}
};

/* F_USB20HO device module definition */
MODULE_ALIAS("platform:f_usb20ho_ohci");	/**<  */
