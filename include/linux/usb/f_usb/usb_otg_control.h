/*
 * include/linux/usb/f_usb/usb_otg_control.h
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
 * include/linux/usb/scd/usb_otg_control.h
 *
 * USB controller driver OTG API
 *
 * Copyright 2005,2006,2008,2011 Sony Corporation
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

#ifndef _USB_OTG_CONTROL_H_
#define _USB_OTG_CONTROL_H_

#include<linux/udif/types.h>


enum usb_otg_vbus_stat {
	USB_OTG_VBUS_STAT_OFF = 0,	/**< OFF   = 0 */
	USB_OTG_VBUS_STAT_LOW,		/**< LOW   = 1 */
	USB_OTG_VBUS_STAT_SESS, 	/**< SESS  = 2 */
	USB_OTG_VBUS_STAT_VALID,	/**< VALID = 3 */
};


enum usb_otg_control_speed {
	USB_OTG_SPEED_LS,		/**< Low Speed  */
	USB_OTG_SPEED_FS,		/**< Full Speed */
	USB_OTG_SPEED_HS,		/**< High Speed */
};


enum usb_otg_control_test_mode {
	USB_OTG_TEST_MODE_NORMAL,

	USB_OTG_TEST_MODE_G_PACKET,
	USB_OTG_TEST_MODE_G_K,
	USB_OTG_TEST_MODE_G_J,
	USB_OTG_TEST_MODE_G_SE0_NAK,

	USB_OTG_TEST_MODE_G_FORCE_PACKET,

	USB_OTG_TEST_MODE_H_PACKET,
	USB_OTG_TEST_MODE_H_K,
	USB_OTG_TEST_MODE_H_J,
	USB_OTG_TEST_MODE_H_SE0_NAK,

	USB_OTG_TEST_MODE_H_SETUP,
	USB_OTG_TEST_MODE_H_IN,
	USB_OTG_TEST_MODE_H_AS,
	USB_OTG_TEST_MODE_H_LS,
	USB_OTG_TEST_MODE_H_FS,
	USB_OTG_TEST_MODE_H_HS,
};


enum usb_otg_line_state_value {
	USB_OTG_LINE_STATE_LOW = 0,	/**< Low  */
	USB_OTG_LINE_STATE_HIGH = 1U,	/**< High */
};


enum usb_otg_event_type {
	USB_OTG_EVENT_TYPE_CID = 0,
	USB_OTG_EVENT_TYPE_VBUS,
	USB_OTG_EVENT_TYPE_VBUSERROR,
	USB_OTG_EVENT_TYPE_CON,
	USB_OTG_EVENT_TYPE_SRP,
	USB_OTG_EVENT_TYPE_RCHOST,
	USB_OTG_EVENT_TYPE_RCGADGET,
};


struct usb_otg_event_cid {
	enum usb_otg_event_type type;
	unsigned int value;
	unsigned int port;
};


struct usb_otg_event_vbus {
	enum usb_otg_event_type type;
	enum usb_otg_vbus_stat vbus_stat;
	unsigned int port;
};


struct usb_otg_event_vbuserror {
	enum usb_otg_event_type type;
};


struct usb_otg_event_con {
	enum usb_otg_event_type type;
	unsigned int value;
};


struct usb_otg_event_srp {
	enum usb_otg_event_type type;
};


struct usb_otg_event_rc {
	enum usb_otg_event_type type;
	unsigned int status;			/**< RoleChange status */
};

#define USB_OTG_SUCCESS 		0
#define USB_OTG_FAIL_T_B_ASE0_BRST	1U
#define USB_OTG_FAIL_T_A_AIDL_BDIS	2U

union usb_otg_event {
	enum usb_otg_event_type type;
	struct usb_otg_event_cid cid;
	struct usb_otg_event_vbus vbus;
	struct usb_otg_event_vbuserror vbuserror;
	struct usb_otg_event_con con;
	struct usb_otg_event_srp srp;
	struct usb_otg_event_rc rchost;
	struct usb_otg_event_rc rcgadget;
};


struct usb_otg_control_port_info {
	unsigned int nr;
	unsigned int current_port;
};


struct usb_otg_line_state {
	uint8_t dp;			/**< D+ line state	*/
	uint8_t dm;			/**< D- line state	*/
	uint16_t reserved;		/**< Reserved		*/
};



struct usb_otg_control; 		/**< prototype */

struct usb_otg_control_ops {
	int (*get_port_info) (struct usb_otg_control *,struct usb_otg_control_port_info * info);
	int (*select_port) (struct usb_otg_control *, unsigned int pn);
	int (*start_control) (struct usb_otg_control *);
	int (*stop_control) (struct usb_otg_control *);
	int (*start_gadget) (struct usb_otg_control *);
	int (*stop_gadget) (struct usb_otg_control *);
	int (*enable_rchost) (struct usb_otg_control *);
	int (*disable_rchost) (struct usb_otg_control *);
	int (*stop_rchost) (struct usb_otg_control *);
	int (*start_host) (struct usb_otg_control *);
	int (*stop_host) (struct usb_otg_control *);
	int (*start_rcgadget) (struct usb_otg_control *, unsigned int port);
	int (*stop_rcgadget) (struct usb_otg_control *, unsigned int port);
	int (*get_mode) (struct usb_otg_control *);
	int (*request_session) (struct usb_otg_control *);
	int (*set_speed) (struct usb_otg_control *,enum usb_otg_control_speed speed);
	enum usb_otg_control_speed (*get_speed) (struct usb_otg_control *);
	int (*set_test_mode) (struct usb_otg_control *,enum usb_otg_control_test_mode test_mode);
	enum usb_otg_control_test_mode (*get_test_mode) (struct usb_otg_control*);
};


struct usb_otg_control {
	struct usb_otg_control_ops *ops;
	struct usb_otg_core *otg_core;
	const char *name;
	void *data;
};


struct usb_otg_core {
	int (*bind) (struct usb_otg_control * otg_control);
	int (*unbind) (struct usb_otg_control * otg_control);
	int (*notify) (union usb_otg_event * otg_even);
};



struct usb_otg_module_ops {
	struct platform_device *pdev;			/**<  */
	void *data;					/**<  */
	int (*probe) (void *data);			/**<  */
	int (*remove) (void *data);			/**<  */
	int (*resume) (void *data);			/**<  */
	void (*enable_irq) (bool isEnable);		/**<  */
};


struct usb_otg_module_gadget_ops {
	struct platform_device *pdev;			/**<  */
	void *data;					/**<  */
	int (*probe) (void *data);			/**<  */
	int (*remove) (void *data);			/**<  */
	int (*resume) (void *data);			/**<  */
	void (*bus_connect) (unsigned long data);	/**<  */
};



extern int usb_otg_control_register_core(struct usb_otg_core *otg_core);

extern int usb_otg_control_unregister_core(struct usb_otg_core *otg_core);


static inline int
usb_otg_core_notify(struct usb_otg_core *otg_core,
		    union usb_otg_event *otg_event)
{
	return otg_core->notify(otg_event);
}


static inline int
usb_otg_core_bind(struct usb_otg_core *otg_core,
		  struct usb_otg_control *otg_control)
{
	return otg_core->bind(otg_control);
}

static inline int
usb_otg_core_unbind(struct usb_otg_core *otg_core,
		    struct usb_otg_control *otg_control)
{
	return otg_core->unbind(otg_control);
}


static inline int
usb_otg_control_get_port_info(struct usb_otg_control *otg_control,
			      struct usb_otg_control_port_info *info)
{
	if (!info) {
		return -EINVAL;
	}
	return otg_control->ops->get_port_info(otg_control, info);
}


static inline int
usb_otg_control_select_port(struct usb_otg_control *otg_control,
			    unsigned int pn)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->select_port(otg_control, (u32)pn);
}

static inline int
usb_otg_control_start_control(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->start_control(otg_control);
}


static inline int
usb_otg_control_stop_control(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->stop_control(otg_control);
}


static inline int
usb_otg_control_start_gadget(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->start_gadget(otg_control);
}


static inline int
usb_otg_control_stop_gadget(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->stop_gadget(otg_control);
}


static inline int
usb_otg_control_enable_rchost(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->enable_rchost(otg_control);
}


static inline int
usb_otg_control_disable_rchost(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->disable_rchost(otg_control);
}


static inline int
usb_otg_control_stop_rchost(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->stop_rchost(otg_control);
}


static inline int
usb_otg_control_start_host(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->start_host(otg_control);
}


static inline int usb_otg_control_stop_host(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->stop_host(otg_control);
}


static inline int
usb_otg_control_start_rcgadget(struct usb_otg_control *otg_control,
			       unsigned int port)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->start_rcgadget(otg_control, port);
}


static inline int
usb_otg_control_stop_rcgadget(struct usb_otg_control *otg_control,
			      unsigned int port)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->stop_rcgadget(otg_control, port);
}


static inline int usb_otg_control_get_mode(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->get_mode(otg_control);
}

#define USB_OTG_CONTROL_STOP		1U
#define USB_OTG_CONTROL_IDLE		2U
#define USB_OTG_CONTROL_GADGET		3U
#define USB_OTG_CONTROL_RCHOST		4U
#define USB_OTG_CONTROL_HOST		5U
#define USB_OTG_CONTROL_RCGADGET	6U


static inline int
usb_otg_control_request_session(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->request_session(otg_control);
}


static inline int
usb_otg_control_set_speed(struct usb_otg_control *otg_control,
			  unsigned int speed)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->set_speed(otg_control, speed);
}

#define USB_OTG_HS_DISABLE	0
#define USB_OTG_HS_ENABLE	1U


static inline enum usb_otg_control_speed
usb_otg_control_get_speed(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->get_speed(otg_control);
}


static inline int
usb_otg_control_set_test_mode(struct usb_otg_control *otg_control,
			      enum usb_otg_control_test_mode test_mode)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->set_test_mode(otg_control, test_mode);
}


static inline enum usb_otg_control_test_mode
usb_otg_control_get_test_mode(struct usb_otg_control *otg_control)
{
	if (!otg_control) {
		return -EINVAL;
	}
	return otg_control->ops->get_test_mode(otg_control);
}

#define USB_IOCTL_GET_LINE_STATE	1U

extern void *fusb_otg_get_base_addr(unsigned int flag);
#define FUSB_ADDR_TYPE_HOST		0
#define FUSB_ADDR_TYPE_GADGET		1U
#define FUSB_ADDR_TYPE_HDMAC		2U
#define FUSB_ADDR_TYPE_H2XBB		3U
extern enum usb_otg_vbus_stat fusb_otg_get_vbus(void);
extern int fusb_otg_notify_vbuserror(void);
extern int fusb_otg_notify_connect(unsigned int con);
#define FUSB_EVENT_DISCONNECT		0
#define FUSB_EVENT_CONNECT		1U
extern void fusb_otg_host_ehci_bind(struct usb_otg_module_ops *ops);
extern void fusb_otg_host_ohci_bind(struct usb_otg_module_ops *ops);
extern void fusb_otg_gadget_bind(struct usb_otg_module_gadget_ops *ops);
extern unsigned int fusb_otg_gadget_get_hs_enable(void);
extern void fusb_otg_resume_initial_set(void);
#define FUSB_OTG_HOST_EHCI		(1U << 0)
#define FUSB_OTG_HOST_OHCI		(1U << 1)
extern void fusb_otg_host_resume_finish(unsigned int host);
extern void fusb_otg_host_enable_irq(unsigned int host, bool isEnable);

#endif			/* _USB_OTG_CONTROL_H_ */
