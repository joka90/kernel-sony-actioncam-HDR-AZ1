/*
 * drivers/usb/f_usb/usb_otg.h
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

/*--------------------------------------------------------------------------*
 * drivers/usb/scd/scd_device.c
 *
 * OTG and, Platform bus driver operations
 *
 * Copyright 2011 Sony Corporation
 *
 * This file is part of the HS-OTG Controller Driver SCD.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *---------------------------------------------------------------------------*/

#ifndef _USB_OTG_H_
#define _USB_OTG_H_

#define FUSB_VERSION			20120518U

#define FUSB_OTG_OK			0

#define FUSB_OTG_FLAG_OFF		0
#define FUSB_OTG_FLAG_ON		1U

enum {
	FUSB_OTG_REG_CH = 0,
	FUSB_HO_REG_CH,
	FUSB_HDC_REG_CH,
	FUSB_HDMAC_REG_CH,
	FUSB_FVC2_H2XBB_REG_CH,
	FUSB_CLKRST_4_CH,
	FUSB_USB_VBUS_CH,
	FUSB_USB_ID_CH,
	FUSB_CH_MAX_NUM,
};

#define FUSB_PROC_NAME			"fusb"

#define FUSB_A_DEVICE			0
#define FUSB_B_DEVICE			1

#define FUSB_VBUS_ON			0
#define FUSB_VBUS_OFF			1U

#define FUSB_IPRESET_MS_TIME		1U

#define FUSB_DEVICE_CONNETCT_TIMER	100U

#define FUSB_INITIAL_BASE		((void *)(0xFFFF8000U))

#define current_otg_mode		g_fusb_device_control->otg_mode

#ifndef TEST_USB_CNTL_DRV
/* Release   */
#define fusb_lock(lock) 		udif_spin_lock(lock)
#define fusb_unlock(lock)		udif_spin_unlock(lock)
#else  /* #ifndef TEST_FUSB_DRV */
/* USB Controller Driver Test */
#define fusb_lock(lock) {						\
		fusb_lock_cnt++;					\
		udif_spin_lock(lock);					\
	}
#define fusb_unlock(lock) {						\
		udif_spin_unlock(lock); 				\
		fusb_lock_cnt--;					\
	}
#endif /* #ifndef TEST_GPIO_DRV */

struct fusb_device_control {
	UDIF_LIST event_anchor;
	struct usb_otg_control otgc;

	UDIF_VA otg_regs;
	UDIF_VA host_regs;
	UDIF_VA gadget_regs;
	UDIF_VA hdmac_reg;
	UDIF_VA h2xbb_reg;
	UDIF_VA clkrst_4_reg;

	UDIF_SPINLOCK lock;
	UDIF_MUTEX state_mutex;
	UDIF_TASKLET event_tasklet;			/**< tasklet   */
	UDIF_IRQ usb_vbus_irqn;				/**< VBUS IRQ  */
	UDIF_IRQ usb_id_irqn;				/**< ID IRQ    */

	int otg_mode;
	int cid;
	enum usb_otg_vbus_stat vbus;
	int gadget_con;
	int usb_gadget_highspeed_enable;
	int init_comp_flag;
	volatile unsigned int host_resume_comp_flag;

	struct timer_list con_timer;
	struct usb_otg_module_ops *ehci_ops;		/**< ehci ops   */
	struct usb_otg_module_ops *ohci_ops;		/**< ohci ops   */
	struct usb_otg_module_gadget_ops *gadget_ops;	/**< gadget ops */
};

struct fusb_otg_event_container {
	union usb_otg_event event;	/**< event      */
	UDIF_LIST event_list;		/**< event list */
};

static inline struct fusb_device_control *otgc_to_fusb(struct usb_otg_control *otgc) {
	return container_of(otgc, struct fusb_device_control, otgc);
}

static int fusb_read_proc(UDIF_PROC_READ *proc);

static UDIF_ERR fusb_otg_init(UDIF_VP data);
static UDIF_ERR fusb_otg_exit(UDIF_VP data);
static UDIF_ERR fusb_otg_probe(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data);
static UDIF_ERR fusb_otg_remove(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data);
static UDIF_ERR fusb_otg_suspend(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data);
static UDIF_ERR fusb_otg_resume(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data);
static UDIF_ERR fusb_otg_shutdown(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data);

static void fusb_signal_event(UDIF_ULONG data);
static void fusb_signal_event_interrupt(struct fusb_device_control *fusb, union usb_otg_event *event);
static UDIF_ERR fusb_interrupt_usb_id(UDIF_INTR *data);
static UDIF_ERR fusb_interrupt_usb_vbus(UDIF_INTR *data);
static void fusb_notify_cid(struct fusb_device_control *fusb, int current_cid);
static void fusb_notify_vbus(struct fusb_device_control *fusb, enum usb_otg_vbus_stat current_vbus);

static int fusb_get_port_info(struct usb_otg_control *otg_control,
			      struct usb_otg_control_port_info *info);
static int fusb_select_port(struct usb_otg_control *otg_control, unsigned int pn);
static int fusb_start_control(struct usb_otg_control *otg_control);
static int fusb_stop_control(struct usb_otg_control *otg_control);
static int fusb_start_gadget(struct usb_otg_control *otg_control);
static int fusb_stop_gadget(struct usb_otg_control *otg_control);
static int fusb_enable_rchost(struct usb_otg_control *otg_control);
static int fusb_disable_rchost(struct usb_otg_control *otg_control);
static int fusb_stop_rchost(struct usb_otg_control *otg_control);
static int fusb_start_host(struct usb_otg_control *otg_control);
static int fusb_stop_host(struct usb_otg_control *otg_control);
static int fusb_start_rcgadget(struct usb_otg_control *otg_control, unsigned int port);
static int fusb_stop_rcgadget(struct usb_otg_control *otg_control, unsigned int port);
static int fusb_get_mode(struct usb_otg_control *otg_control);
static int fusb_request_session(struct usb_otg_control *otg_control);
static int fusb_set_speed(struct usb_otg_control *otg_control, unsigned int speed);
static enum usb_otg_control_speed fusb_get_speed(struct usb_otg_control *otg_control);
static int fusb_set_test_mode(struct usb_otg_control *otg_control,
			      enum usb_otg_control_test_mode test_mode);
static enum usb_otg_control_test_mode fusb_get_test_mode(struct usb_otg_control *otg_control);

static int fusb_try_start_control(struct fusb_device_control *fusb);
static int fusb_try_stop_control(struct fusb_device_control *fusb);
static int fusb_try_start_gadget(struct fusb_device_control *fusb);
static int fusb_try_stop_gadget(struct fusb_device_control *fusb);
static int fusb_try_start_host(struct fusb_device_control *fusb);
static int fusb_try_stop_host(struct fusb_device_control *fusb);
static void fusb_con_timer_func(unsigned long data);

#endif  /* _USB_OTG_H_ */
