/*
 * drivers/usb/f_usb/usb_otg.c
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#include <linux/udif/module.h>
#include <linux/udif/device.h>
#include <linux/udif/types.h>
#include <linux/udif/irq.h>
#include <linux/udif/io.h>
#include <linux/udif/spinlock.h>
#include <linux/udif/list.h>
#include <linux/udif/tasklet.h>
#include <linux/udif/malloc.h>
#include <linux/udif/mutex.h>
#include <linux/udif/delay.h>
#include <linux/udif/proc.h>

#include "linux/usb/f_usb/usb_otg_control.h"
#include "usb_otg.h"
#include "usb_otg_reg.h"

//#define FUSB_FUNC_TRACE					/**<  */

#ifdef FUSB_FUNC_TRACE
#define fusb_func_trace(b) printk("CALL %s %s(%d)\n",(b),__FUNCTION__,__LINE__)
#define fusb_print_dbg(fmt, arg...) printk(KERN_DEBUG fmt " %s(%d)\n", ##arg, __FUNCTION__,__LINE__)
#define fusb_print_err(fmt, arg...) printk(KERN_ERR fmt " %s(%d)\n", ##arg, __FUNCTION__,__LINE__)
#else
#define fusb_func_trace(b)
#define fusb_print_dbg(fmt, arg...)
#define fusb_print_err(fmt, arg...)
#endif

#ifdef NDEBUG
#define fusb_assert_NULL(b)
#else /* #ifdef NDEBUG */
#define fusb_assert_NULL(b)								\
	if (!(b))									\
	{										\
		printk(KERN_ERR " %s(%d) 0x%x *** ASSERT \n",__FILE__,__LINE__,(b));	\
		BUG();									\
	}				 /**<  */
#endif


/**
 * @brief usb_str
 *
 *
 */
static char* usb_str = "fusb_otg";
UDIF_MODULE_PARAM(usb_str, charp, 0664U);

UDIF_IDS(fusb_otg_ids) = {
	UDIF_ID(UDIF_ID_FUSB_OTG, 0xFFU)			/* 0b:USB_VBUS, 1b:USB_ID, 2b:OTG Register I/O, 3b:Ho Register I/O */
};								/* 4b:HDC Register I/O, 5b:HDMAC Register, 6b:FVC2_H2XBB Register, 7b:CLKRST_4 Register */

static UDIF_DRIVER_OPS fusb_otg_cb = {
	.init       = fusb_otg_init,
	.exit       = fusb_otg_exit,
	.probe      = fusb_otg_probe,
	.remove     = fusb_otg_remove,
	.suspend    = fusb_otg_suspend,
	.resume     = fusb_otg_resume,
	.shutdown   = fusb_otg_shutdown,
};

UDIF_MODULE(fusb_otg, "fusb_otg", "1.0", fusb_otg_cb, fusb_otg_ids, NULL, NULL);

static struct fusb_device_control *g_fusb_device_control = NULL;

static UDIF_LIST_HEAD(fusb_anchor);

static UDIF_DECLARE_SPINLOCK(fusb_lock);

static int fusb_create_proc_flag = FUSB_OTG_FLAG_OFF;

static struct usb_otg_control_ops otg_control_ops = {
	.get_port_info      = fusb_get_port_info,
	.select_port        = fusb_select_port,
	.start_control      = fusb_start_control,
	.stop_control       = fusb_stop_control,
	.start_gadget       = fusb_start_gadget,
	.stop_gadget        = fusb_stop_gadget,
	.enable_rchost      = fusb_enable_rchost,
	.disable_rchost     = fusb_disable_rchost,
	.stop_rchost        = fusb_stop_rchost,
	.start_host         = fusb_start_host,
	.stop_host          = fusb_stop_host,
	.start_rcgadget     = fusb_start_rcgadget,
	.stop_rcgadget      = fusb_stop_rcgadget,
	.get_mode           = fusb_get_mode,
	.request_session    = fusb_request_session,
	.set_speed          = fusb_set_speed,
	.get_speed          = fusb_get_speed,
	.set_test_mode      = fusb_set_test_mode,
	.get_test_mode      = fusb_get_test_mode,
};


static int fusb_read_proc(UDIF_PROC_READ *proc)
{
	int                     len = 0;

	if (proc == NULL) {
		fusb_print_err("argument proc is set NULL");
		return UDIF_ERR_PAR;
	}

	len += udif_proc_setbuf(proc, len, "USB Controller Driver Ver : %d\n", FUSB_VERSION);

	udif_proc_setend(proc);
	return len;
}

/**
 * @brief fusb_proc
 *
 *
 */
static UDIF_PROC fusb_proc = {
	.name  = FUSB_PROC_NAME,
	.read  = fusb_read_proc
};

static UDIF_ERR fusb_otg_init(UDIF_VP data)
{
	struct fusb_device_control *fusb;
	UDIF_ERR ret;

	fusb_func_trace("START");

	fusb = udif_kmalloc("fusb_dev_ctrl", sizeof(struct fusb_device_control), 0);
	if (fusb == NULL) {
		fusb_print_err("udif_kmalloc is failed");
		return UDIF_ERR_NOMEM;
	}

	fusb->otgc.ops      = &otg_control_ops;
	fusb->otg_regs      = NULL;
	fusb->host_regs     = NULL;
	fusb->gadget_regs   = NULL;
	fusb->hdmac_reg     = NULL;
	fusb->h2xbb_reg     = NULL;
	fusb->clkrst_4_reg  = NULL;
	udif_spin_lock_init(&fusb->lock);
	udif_mutex_init(&fusb->state_mutex);
	udif_tasklet_init(&fusb->event_tasklet, fusb_signal_event, (UDIF_ULONG)fusb);
	udif_list_head_init(&fusb->event_anchor);
	fusb->otg_mode      = USB_OTG_CONTROL_STOP;
	fusb->cid           = FUSB_B_DEVICE;
	fusb->vbus          = FUSB_VBUS_OFF;
	fusb->gadget_con    = FUSB_EVENT_DISCONNECT;
	fusb->usb_gadget_highspeed_enable = USB_OTG_HS_ENABLE;
	fusb->init_comp_flag = FUSB_OTG_FLAG_ON;
	fusb->host_resume_comp_flag = 0;

	init_timer(&fusb->con_timer);
	fusb->con_timer.function    = fusb_con_timer_func;

	fusb->ehci_ops = NULL;
	fusb->ohci_ops = NULL;

	g_fusb_device_control = fusb;

	if ((ret = udif_create_proc(&fusb_proc)) != UDIF_ERR_OK) {
		fusb_print_err("udif_create_proc is failed");
		return ret;
	}
	fusb_create_proc_flag = FUSB_OTG_FLAG_ON;

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static UDIF_ERR fusb_otg_exit(UDIF_VP data)
{
	struct fusb_device_control *fusb = g_fusb_device_control;
	UDIF_ERR ret;

	fusb_func_trace("START");

	if (fusb_create_proc_flag == FUSB_OTG_FLAG_ON) {
		if ((ret = udif_remove_proc(&fusb_proc)) != UDIF_ERR_OK) {
			fusb_print_err("udif_remove_proc is failed");
		} else {
			fusb_create_proc_flag = FUSB_OTG_FLAG_OFF;
		}
	}

	if (g_fusb_device_control == NULL) {
		return UDIF_ERR_OK;
	}

	g_fusb_device_control = NULL;

	udif_tasklet_kill(&fusb->event_tasklet);
	udif_kfree((UDIF_VP)fusb);

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static UDIF_ERR fusb_otg_probe(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data)
{
	struct fusb_device_control *fusb;
	unsigned int status;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return UDIF_ERR_NOENT;
	}

	switch(ch) {
	case FUSB_USB_VBUS_CH:
		fusb->usb_vbus_irqn = udif_devint_irq(dev, ch);
		udif_request_irq(dev, ch, fusb_interrupt_usb_vbus, (UDIF_VP *)fusb);
		udif_disable_irq(fusb->usb_vbus_irqn);
		set_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_INTHE_OFFSET, FUSB_DEF_USB_VBUS_INTHE);
		set_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_INTLE_OFFSET, FUSB_DEF_USB_VBUS_INTLE);
		break;
	case FUSB_USB_ID_CH:
		fusb->usb_id_irqn = udif_devint_irq(dev, ch);
		udif_request_irq(dev, ch, fusb_interrupt_usb_id, (UDIF_VP *)fusb);
		udif_disable_irq(fusb->usb_id_irqn);
		set_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_INTHE_OFFSET, FUSB_DEF_USB_ID_INTHE);
		set_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_INTLE_OFFSET, FUSB_DEF_USB_ID_INTLE);
		break;
	case FUSB_OTG_REG_CH:
		fusb->otg_regs = udif_devio_virt(dev, ch);
		break;
	case FUSB_HO_REG_CH:
		fusb->host_regs = udif_devio_virt(dev, ch);
		break;
	case FUSB_HDC_REG_CH:
		fusb->gadget_regs = udif_devio_virt(dev, ch);
		break;
	case FUSB_HDMAC_REG_CH:
		fusb->hdmac_reg = udif_devio_virt(dev, ch);
		break;
	case FUSB_FVC2_H2XBB_REG_CH:
		fusb->h2xbb_reg = udif_devio_virt(dev, ch);
		break;
	case FUSB_CLKRST_4_CH:
		fusb->clkrst_4_reg = udif_devio_virt(dev, ch);
		write_reg_param(fusb->clkrst_4_reg, FUSB_REG_IPRESET0_SET_OFFSET, FUSB_DEF_RRST_USB);
		status = get_reg_param(fusb->clkrst_4_reg, FUSB_REG_IPRESET0_SET_OFFSET, FUSB_DEF_RRST_USB);
		udif_msleep(FUSB_IPRESET_MS_TIME);
		break;
	default :
		fusb_print_err("Out of setting channel");
		return UDIF_ERR_PAR;
	}

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static UDIF_ERR fusb_otg_remove(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return UDIF_ERR_NOENT;
	}

	switch(ch) {
	case FUSB_USB_VBUS_CH:
	case FUSB_USB_ID_CH:
		udif_free_irq(dev, ch);
		break;

	case FUSB_OTG_REG_CH:
		fusb->otg_regs = NULL;
		break;
	case FUSB_HO_REG_CH:
		fusb->host_regs = NULL;
		break;
	case FUSB_HDC_REG_CH:
		fusb->gadget_regs = NULL;
		break;
	case FUSB_HDMAC_REG_CH:
		fusb->hdmac_reg = NULL;
		break;
	case FUSB_FVC2_H2XBB_REG_CH:
		fusb->h2xbb_reg = NULL;
		break;
	case FUSB_CLKRST_4_CH:
		fusb->clkrst_4_reg = NULL;
		break;
	default :
		fusb_print_err("Out of setting channel");
		return UDIF_ERR_PAR;
	}

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static UDIF_ERR fusb_otg_suspend(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return UDIF_ERR_NOENT;
	}

	fusb->init_comp_flag = FUSB_OTG_FLAG_OFF;

	if (current_otg_mode == USB_OTG_CONTROL_STOP) {
		return UDIF_ERR_OK;
	}

	switch(ch) {
	case FUSB_USB_VBUS_CH:
		udif_disable_irq(fusb->usb_vbus_irqn);
		break;
	case FUSB_USB_ID_CH:
		udif_disable_irq(fusb->usb_id_irqn);
		break;
	default :
		break;
	}

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static UDIF_ERR fusb_otg_resume(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data)
{
	struct fusb_device_control *fusb;
	int cid;
	enum usb_otg_vbus_stat vbus;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return UDIF_ERR_NOENT;
	}

	fusb_otg_resume_initial_set();

	set_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_INTHE_OFFSET, FUSB_DEF_USB_VBUS_INTHE);
	set_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_INTLE_OFFSET, FUSB_DEF_USB_VBUS_INTLE);
	set_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_INTHE_OFFSET, FUSB_DEF_USB_ID_INTHE);
	set_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_INTLE_OFFSET, FUSB_DEF_USB_ID_INTLE);

	if (current_otg_mode == USB_OTG_CONTROL_STOP) {
		return UDIF_ERR_OK;
	}

	switch(ch) {
	case FUSB_USB_VBUS_CH:
		udif_enable_irq(fusb->usb_vbus_irqn);
		vbus = get_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_ID_EXT_OFFSET, FUSB_DEF_USB_VBUS);
		fusb_notify_vbus(fusb, vbus);
		break;
	case FUSB_USB_ID_CH:
		udif_enable_irq(fusb->usb_id_irqn);
		if (get_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_ID_EXT_OFFSET, FUSB_DEF_USB_ID)) {
			cid = FUSB_B_DEVICE;
		} else {
			cid = FUSB_A_DEVICE;
		}
		fusb_notify_cid(fusb, cid);
		break;
	default :
		break;
	}

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static UDIF_ERR fusb_otg_shutdown(const UDIF_DEVICE * dev, UDIF_CH ch, UDIF_VP data)
{
	struct fusb_device_control *fusb = g_fusb_device_control;
	UDIF_ERR ret;

	fusb_func_trace("START");

	if (fusb_create_proc_flag == FUSB_OTG_FLAG_ON) {
		if ((ret = udif_remove_proc(&fusb_proc)) != UDIF_ERR_OK) {
			fusb_print_err("udif_remove_proc is failed");
		} else {
			fusb_create_proc_flag = FUSB_OTG_FLAG_OFF;
		}
	}

	if (g_fusb_device_control == NULL) {
		return UDIF_ERR_OK;
	}

	g_fusb_device_control = NULL;

	udif_tasklet_kill(&fusb->event_tasklet);
	udif_kfree((UDIF_VP)fusb);

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static void fusb_signal_event(UDIF_ULONG data)
{
	struct fusb_device_control *fusb;

	UDIF_LIST temp_anchor;
	struct fusb_otg_event_container *ec;
	struct fusb_otg_event_container *temp_ec;

	if (data == 0) {
		return;
	}

	fusb = (struct fusb_device_control *)data;

	udif_list_head_init(&temp_anchor);
	fusb_lock(&fusb->lock);
	udif_list_for_each_entry_safe(ec, temp_ec, &fusb->event_anchor, event_list) {
		udif_list_del(&ec->event_list);
		udif_list_head_init(&ec->event_list);
		udif_list_add_tail(&ec->event_list, &temp_anchor);
	}
	fusb_unlock(&fusb->lock);

	udif_list_for_each_entry_safe(ec, temp_ec, &temp_anchor, event_list) {
		udif_list_del(&ec->event_list);

		if ((ec->event.type           == USB_OTG_EVENT_TYPE_VBUS ) &&
		    (current_otg_mode         == USB_OTG_CONTROL_GADGET  )) {
			if ((fusb->gadget_ops != NULL) && (fusb->gadget_ops->bus_connect != NULL)) {
				fusb->gadget_ops->bus_connect((unsigned long)fusb->gadget_ops->data);
			}
		}

		if (fusb->otgc.otg_core != NULL) {
			fusb_print_dbg("usb_otg_core_notify. event type = %d \n",ec->event.type);
			usb_otg_core_notify(fusb->otgc.otg_core, &ec->event);
		}
		udif_kfree(ec);
	}

	return;
}

static void fusb_signal_event_interrupt(struct fusb_device_control *fusb, union usb_otg_event *event)
{
	struct fusb_otg_event_container *event_container;

	event_container = (struct fusb_otg_event_container *)udif_kmalloc("fusb_usb_event", sizeof(struct fusb_otg_event_container), 0);
	if (event_container == NULL) {
		fusb_print_err("udif_kmalloc is failed");
		return;
	}

	memcpy(&event_container->event, event, sizeof(union usb_otg_event));
	udif_list_head_init(&event_container->event_list);
	fusb_lock(&fusb->lock);
	udif_list_add_tail(&event_container->event_list, &fusb->event_anchor);
	fusb_unlock(&fusb->lock);

	udif_tasklet_schedule(&fusb->event_tasklet);

	return;
}

static UDIF_ERR fusb_interrupt_usb_id(UDIF_INTR *intr)
{
	struct fusb_device_control *fusb;
	int cid;

	fusb_func_trace("START");

	if (intr == NULL) {
		return UDIF_ERR_PAR;
	}

	fusb = (struct fusb_device_control *)udif_intr_data(intr);

	if (get_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_ID_EXT_OFFSET, FUSB_DEF_USB_ID) == FUSB_DEF_USB_ID) {
		cid = FUSB_B_DEVICE;
	} else {
		cid = FUSB_A_DEVICE;
	}
	fusb_notify_cid(fusb, cid);

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static UDIF_ERR fusb_interrupt_usb_vbus(UDIF_INTR *intr)
{
	struct fusb_device_control *fusb;
	int vbus;

	fusb_func_trace("START");

	if (intr == NULL) {
		return UDIF_ERR_PAR;
	}

	fusb = (struct fusb_device_control *)udif_intr_data(intr);

	vbus  = get_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_ID_EXT_OFFSET, FUSB_DEF_USB_VBUS);
	fusb_notify_vbus(fusb, vbus);

	fusb_func_trace("END");

	return UDIF_ERR_OK;
}

static void fusb_notify_cid(struct fusb_device_control *fusb, int current_cid)
{
	union usb_otg_event otg_event;

	if (fusb->cid != current_cid) {
		otg_event.type              = USB_OTG_EVENT_TYPE_CID;
		otg_event.cid.value         = current_cid;
		otg_event.cid.port          = 0;
		fusb_signal_event_interrupt(fusb, &otg_event);
		fusb->cid = current_cid;
		fusb_print_dbg("current_cid = %d \n",current_cid);
	}
}

static void fusb_notify_vbus(struct fusb_device_control *fusb, enum usb_otg_vbus_stat current_vbus)
{
	union usb_otg_event otg_event;

	if (fusb->vbus != current_vbus) {
		otg_event.type              = USB_OTG_EVENT_TYPE_VBUS;
		if (current_vbus == FUSB_VBUS_ON) {
			otg_event.vbus.vbus_stat = USB_OTG_VBUS_STAT_VALID;
		} else {
			otg_event.vbus.vbus_stat = USB_OTG_VBUS_STAT_OFF;
		}
		otg_event.vbus.port         = 0;
		fusb_signal_event_interrupt(fusb, &otg_event);
		fusb->vbus = current_vbus;
		if( fusb->vbus == FUSB_VBUS_OFF ){
			fusb->gadget_con = FUSB_EVENT_DISCONNECT;
		}
	}
}

static int fusb_get_port_info(struct usb_otg_control *otg_control,
			      struct usb_otg_control_port_info *info)
{
	fusb_func_trace("START");

	if ((otg_control == NULL)||(info == NULL)) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	info->nr            = 0;
	info->current_port  = 0;

	fusb_func_trace("END");

	return FUSB_OTG_OK;
}

static int fusb_select_port(struct usb_otg_control *otg_control, unsigned int pn)
{
	fusb_func_trace(" ");

	return FUSB_OTG_OK;
}

static int fusb_start_control(struct usb_otg_control *otg_control)
{
	int status;
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);
	if (current_otg_mode != USB_OTG_CONTROL_STOP) {
		fusb_print_err("OTG MODE is not STOP MODE");
		return -EBUSY;
	}

	status = fusb_try_start_control(fusb);

	fusb_func_trace("END");

	return status ;
}

static int fusb_stop_control(struct usb_otg_control *otg_control)
{
	int status;
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);
	if (current_otg_mode != USB_OTG_CONTROL_IDLE) {
		fusb_print_err("OTG MODE is not IDLE MODE");
		return -EBUSY;
	}

	status = fusb_try_stop_control(fusb);

	fusb_func_trace("END");

	return status;
}

static int fusb_start_gadget(struct usb_otg_control *otg_control)
{
	int status;
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);
	if (current_otg_mode != USB_OTG_CONTROL_IDLE) {
		fusb_print_err("OTG MODE is not IDLE MODE");
		return -EBUSY;
	}

	status = fusb_try_start_gadget(fusb);

	fusb_func_trace("END");

	return status;
}

static int fusb_stop_gadget(struct usb_otg_control *otg_control)
{
	int status;
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);
	if (current_otg_mode != USB_OTG_CONTROL_GADGET) {
		fusb_print_err("OTG MODE is not GADGET MODE");
		return -EBUSY;
	}

	status = fusb_try_stop_gadget(fusb);

	fusb_func_trace("END");

	return status;
}

static int fusb_enable_rchost(struct usb_otg_control *otg_control)
{
	fusb_func_trace(" ");

	return FUSB_OTG_OK;
}

static int fusb_disable_rchost(struct usb_otg_control *otg_control)
{
	fusb_func_trace(" ");

	return FUSB_OTG_OK;
}

static int fusb_stop_rchost(struct usb_otg_control *otg_control)
{
	fusb_func_trace(" ");

	return FUSB_OTG_OK;
}

static int fusb_start_host(struct usb_otg_control *otg_control)
{
	int status;
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);
	if (current_otg_mode != USB_OTG_CONTROL_IDLE) {
		fusb_print_err("OTG MODE is not IDLE MODE");
		return -EBUSY;
	}

	status = fusb_try_start_host(fusb);

	fusb_func_trace("END");

	return status;
}

static int fusb_stop_host(struct usb_otg_control *otg_control)
{
	int status;
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);
	if (current_otg_mode != USB_OTG_CONTROL_HOST) {
		fusb_print_err("OTG MODE is not HOST MODE");
		return -EBUSY;
	}

	status = fusb_try_stop_host(fusb);

	fusb_func_trace("END");

	return status;
}

static int fusb_start_rcgadget(struct usb_otg_control *otg_control, unsigned int port)
{
	fusb_func_trace(" ");

	return FUSB_OTG_OK;
}

static int fusb_stop_rcgadget(struct usb_otg_control *otg_control, unsigned int port)
{
	fusb_func_trace(" ");

	return FUSB_OTG_OK;
}

static int fusb_get_mode(struct usb_otg_control *otg_control)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);

	fusb_func_trace("END");

	return current_otg_mode;
}

static int fusb_request_session(struct usb_otg_control *otg_control)
{
	fusb_func_trace(" ");

	return FUSB_OTG_OK;
}

static int fusb_set_speed(struct usb_otg_control *otg_control, unsigned int speed)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	if ((speed != USB_OTG_HS_ENABLE) && (speed != USB_OTG_HS_DISABLE)) {
		fusb_print_err("Out of setting argument");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);

	fusb->usb_gadget_highspeed_enable = speed;

	fusb_func_trace("END");

	return FUSB_OTG_OK;
}

static enum usb_otg_control_speed fusb_get_speed(struct usb_otg_control *otg_control)
{
	struct fusb_device_control *fusb;
	enum usb_otg_control_speed speed;
	unsigned int port_owner;
	unsigned int lspeed;
	unsigned int hspeed;

	fusb_func_trace("START");

	if (otg_control == NULL) {
		fusb_print_err("argument is set NULL");
		return -EINVAL;
	}

	fusb = otgc_to_fusb(otg_control);
	if (fusb->cid == FUSB_A_DEVICE) {
		port_owner = get_reg_param(fusb->host_regs, FUSB_REG_EHCI_PORTSC_1_OFFSET, FUSB_DEF_PORT_OWNER);
		if (port_owner == FUSB_DEF_PORT_OWNER) {
			lspeed = get_reg_param(fusb->host_regs, FUSB_REG_OHCI_HC_RH_PORT_STATUS, FUSB_DEF_LOW_SPEED_DEVICE_ATTACHED);
			if (lspeed == FUSB_DEF_LOW_SPEED_DEVICE_ATTACHED) {
				speed = USB_OTG_SPEED_LS;
			} else {
				speed = USB_OTG_SPEED_FS;
			}
		} else {
			speed = USB_OTG_SPEED_HS;
		}
	} else {
		hspeed = get_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_DEVICE_OFFSET, FUSB_DEF_M_HSPEED_O);
		if (hspeed == FUSB_DEF_M_HSPEED_O) {
			speed = USB_OTG_SPEED_HS;
		} else {
			speed = USB_OTG_SPEED_FS;
		}
	}

	fusb_func_trace("END");

	return speed;
}

static int fusb_set_test_mode(struct usb_otg_control *otg_control,
			      enum usb_otg_control_test_mode test_mode)
{
	fusb_func_trace(" ");

	return FUSB_OTG_OK;
}

static enum usb_otg_control_test_mode fusb_get_test_mode(struct usb_otg_control *otg_control)
{
	fusb_func_trace(" ");

	return USB_OTG_TEST_MODE_NORMAL;
}

static int fusb_try_start_control(struct fusb_device_control *fusb)
{

	int cid;
	enum usb_otg_vbus_stat vbus;

	fusb_assert_NULL(fusb != NULL);

	udif_enable_irq(fusb->usb_id_irqn);
	udif_enable_irq(fusb->usb_vbus_irqn);

	if (get_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_ID_EXT_OFFSET, FUSB_DEF_USB_ID)) {
		cid = FUSB_B_DEVICE;
	} else {
		cid = FUSB_A_DEVICE;
	}
	vbus    = get_reg_param(fusb->otg_regs, FUSB_REG_OTG_USB_ID_EXT_OFFSET, FUSB_DEF_USB_VBUS);

	fusb_notify_cid(fusb, cid);
	fusb_notify_vbus(fusb, vbus);

	current_otg_mode = USB_OTG_CONTROL_IDLE;

	return 0;
}

static int fusb_try_stop_control(struct fusb_device_control *fusb)
{
	fusb_assert_NULL(fusb != NULL);

	udif_disable_irq(fusb->usb_id_irqn);
	udif_disable_irq(fusb->usb_vbus_irqn);

	udif_tasklet_kill(&fusb->event_tasklet);

	current_otg_mode = USB_OTG_CONTROL_STOP;

	return FUSB_OTG_OK;
}

static int fusb_try_start_gadget(struct fusb_device_control *fusb)
{
	unsigned int port_power;
	unsigned int status;

	if ((fusb->cid != FUSB_B_DEVICE) || (fusb->vbus != FUSB_VBUS_ON)) {
		fusb_print_err("Not Condition Start Gadget ");
		return -EBUSY;
	}

	port_power = get_reg_param(fusb->host_regs, FUSB_REG_EHCI_PORTSC_1_OFFSET, FUSB_DEF_PORT_POWER);
	if (port_power) {
		fusb_print_err("Host port power is ON status. must stop HOST");
		return -EBUSY;
	}

	write_reg_param(fusb->otg_regs, FUSB_REG_OTG_UNLIMIT_CLR, FUSB_DEF_USB_HF_SEL);
	status = get_reg_param(fusb->otg_regs, FUSB_REG_OTG_UNLIMIT, FUSB_DEF_USB_HF_SEL);

	if ((fusb->gadget_ops != NULL) && (fusb->gadget_ops->probe != NULL)) {
		fusb->gadget_ops->probe(fusb->gadget_ops->data);
	}

	current_otg_mode = USB_OTG_CONTROL_GADGET;

	return FUSB_OTG_OK;
}

static int fusb_try_stop_gadget(struct fusb_device_control *fusb)
{
	if ((fusb->gadget_ops != NULL) && (fusb->gadget_ops->remove != NULL)) {
		fusb->gadget_ops->remove(fusb->gadget_ops->data);
	}

	current_otg_mode = USB_OTG_CONTROL_IDLE;
	return FUSB_OTG_OK;
}

static int fusb_try_start_host(struct fusb_device_control *fusb)
{
	unsigned int device_disconnect;
	unsigned int status;

	if (fusb->cid != FUSB_A_DEVICE) {
		fusb_print_err("Not Condition Start Host ");
		return -EBUSY;
	}

	device_disconnect = get_reg_param(fusb->gadget_regs, FUSB_REG_GADGET_DEVC_OFFSET, FUSB_DEF_DISCONNECT);
	if (!device_disconnect) {
		fusb_print_err("Gadget status is connect. must stop GADGET");
		return -EBUSY;
	}

	write_reg_param(fusb->otg_regs, FUSB_REG_OTG_UNLIMIT_SET, FUSB_DEF_USB_HF_SEL);
	status = get_reg_param(fusb->otg_regs, FUSB_REG_OTG_UNLIMIT, FUSB_DEF_USB_HF_SEL);

	if ((fusb->ehci_ops != NULL) && (fusb->ehci_ops->probe != NULL)) {
		fusb->ehci_ops->probe(fusb->ehci_ops->data);
	}

	if ((fusb->ohci_ops != NULL) && (fusb->ohci_ops->probe != NULL)) {
		fusb->ohci_ops->probe(fusb->ohci_ops->data);
	}

	current_otg_mode = USB_OTG_CONTROL_HOST;

	return FUSB_OTG_OK;
}

static int fusb_try_stop_host(struct fusb_device_control *fusb)
{
	current_otg_mode = USB_OTG_CONTROL_IDLE;

	if ((fusb->ohci_ops != NULL) && (fusb->ohci_ops->remove != NULL)) {
		fusb->ohci_ops->remove(fusb->ohci_ops->data);
	}

	if ((fusb->ehci_ops != NULL) && (fusb->ehci_ops->remove != NULL)) {
		fusb->ehci_ops->remove(fusb->ehci_ops->data);
	}

	return FUSB_OTG_OK;
}

int usb_otg_control_register_core(struct usb_otg_core* otg_core)
{
	int status;
	struct fusb_device_control *fusb;

	fusb_lock(&fusb_lock);

	fusb_func_trace("START");

	if (otg_core == NULL) {
		fusb_print_err("argument otg_core is set NULL");
		fusb_unlock(&fusb_lock);
		return -EINVAL;
	}

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		fusb_unlock(&fusb_lock);
		return -ENOENT;
	}

	fusb->otgc.otg_core = otg_core;

	status = usb_otg_core_bind(otg_core, &fusb->otgc);
	if (status < 0) {
		fusb_print_dbg("usb_otge_core_bind is failed status=%d \n", status);
		fusb->otgc.otg_core = NULL;
	}

	fusb_unlock(&fusb_lock);

	fusb_func_trace("END");

	return status;
}
EXPORT_SYMBOL(usb_otg_control_register_core);         /**<  */

int usb_otg_control_unregister_core(struct usb_otg_core *otg_core)
{
	int status;
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	if (otg_core == NULL) {
		fusb_print_err("argument otg_core is set NULL");
		return -EINVAL;
	}

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return -ENOENT;
	}

	fusb_lock(&fusb_lock);

	if (fusb->otgc.otg_core != otg_core) {
		fusb_print_err("fusb->otgc.otg_core NULL pointer");
		fusb_unlock(&fusb_lock);
		return -EINVAL;
	}
	fusb->otgc.otg_core = NULL;
	status = usb_otg_core_unbind(otg_core, &fusb->otgc);

	fusb_unlock(&fusb_lock);

	fusb_func_trace("END");

	return status;
}
EXPORT_SYMBOL(usb_otg_control_unregister_core);       /**<  */

void *fusb_otg_get_base_addr(unsigned int flag)
{
	struct fusb_device_control *fusb;
	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return NULL;
	}

	if (flag == FUSB_ADDR_TYPE_HOST) {
		return (void *)fusb->host_regs;
	} else if (flag == FUSB_ADDR_TYPE_GADGET) {
		return (void *)fusb->gadget_regs;
	} else if (flag == FUSB_ADDR_TYPE_HDMAC) {
		return (void *)fusb->hdmac_reg;
	} else if (flag == FUSB_ADDR_TYPE_H2XBB) {
		return (void *)fusb->h2xbb_reg;
	}

	fusb_print_err("Out of setting argument");
	return NULL;
}
EXPORT_SYMBOL(fusb_otg_get_base_addr);                /**<  */

enum usb_otg_vbus_stat fusb_otg_get_vbus(void)
{
	struct fusb_device_control *fusb;
	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return USB_OTG_VBUS_STAT_OFF;
	}

	if (fusb->vbus == FUSB_VBUS_ON) {
		return USB_OTG_VBUS_STAT_VALID;
	}

	return USB_OTG_VBUS_STAT_OFF;
}
EXPORT_SYMBOL(fusb_otg_get_vbus);                     /**<  */

int fusb_otg_notify_vbuserror(void)
{
	struct fusb_device_control *fusb;
	union usb_otg_event otg_event;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return -ENOENT;
	}

	otg_event.type = USB_OTG_EVENT_TYPE_VBUSERROR;
	fusb_signal_event_interrupt(fusb, &otg_event);

	fusb_func_trace("END");

	return FUSB_OTG_OK;
}
EXPORT_SYMBOL(fusb_otg_notify_vbuserror);             /**<  */

static void fusb_con_timer_func(unsigned long data)
{
	struct fusb_device_control *fusb;
	union usb_otg_event otg_event;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return;
	}

	if ((fusb->gadget_con != FUSB_EVENT_CONNECT) && (data == FUSB_EVENT_CONNECT)) {
		fusb->gadget_con = FUSB_EVENT_CONNECT;
		otg_event.type = USB_OTG_EVENT_TYPE_CON;
		otg_event.con.value = 1U;
		fusb_signal_event_interrupt(fusb, &otg_event);
	}

	fusb_func_trace("END");

	return;
}

int fusb_otg_notify_connect(unsigned int con)
{
	struct fusb_device_control *fusb;
	union usb_otg_event otg_event;
	unsigned long expires;

	if ((con != FUSB_EVENT_DISCONNECT) && (con != FUSB_EVENT_CONNECT)) {
		fusb_print_err("Out of setting argument");
		return -EINVAL;
	}

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return -ENOENT;
	}

	fusb->con_timer.data = con;
	if (fusb->gadget_con != con) {
		if (con == FUSB_EVENT_CONNECT) {
			expires = jiffies + msecs_to_jiffies(FUSB_DEVICE_CONNETCT_TIMER);
			mod_timer(&fusb->con_timer, expires);
		} else {
			fusb->gadget_con = con;
			otg_event.type = USB_OTG_EVENT_TYPE_CON;
			otg_event.con.value = 0;
			fusb_signal_event_interrupt(fusb, &otg_event);
		}
	}

	return FUSB_OTG_OK;
}
EXPORT_SYMBOL(fusb_otg_notify_connect);               /**<  */

void fusb_otg_host_ehci_bind(struct usb_otg_module_ops *ops)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return;
	}

	fusb->ehci_ops = ops;

	fusb_func_trace("END");

	return;
}
EXPORT_SYMBOL(fusb_otg_host_ehci_bind);               /**<  */

void fusb_otg_host_ohci_bind(struct usb_otg_module_ops *ops)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return;
	}

	fusb->ohci_ops = ops;

	fusb_func_trace("END");

	return;
}
EXPORT_SYMBOL(fusb_otg_host_ohci_bind);               /**<  */

void fusb_otg_gadget_bind(struct usb_otg_module_gadget_ops *ops)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return;
	}

	fusb->gadget_ops = ops;
	fusb_func_trace("END");

	return;
}
EXPORT_SYMBOL(fusb_otg_gadget_bind);                  /**<  */

unsigned int fusb_otg_gadget_get_hs_enable(void)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return USB_OTG_HS_ENABLE;
	}
	fusb_func_trace("END");
	return fusb->usb_gadget_highspeed_enable;
}
EXPORT_SYMBOL(fusb_otg_gadget_get_hs_enable);         /**<  */

void fusb_otg_resume_initial_set(void)
{
	struct fusb_device_control *fusb;
	unsigned int status;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		return;
	}

	udif_mutex_lock(&fusb->state_mutex);
	if (fusb->init_comp_flag == FUSB_OTG_FLAG_OFF) {
		write_reg_param(fusb->clkrst_4_reg, FUSB_REG_IPRESET0_SET_OFFSET, FUSB_DEF_RRST_USB);
		status = get_reg_param(fusb->clkrst_4_reg, FUSB_REG_IPRESET0_SET_OFFSET, FUSB_DEF_RRST_USB);
		udif_msleep(FUSB_IPRESET_MS_TIME);

		if (current_otg_mode == USB_OTG_CONTROL_HOST) {
			write_reg_param(fusb->otg_regs, FUSB_REG_OTG_UNLIMIT_SET, FUSB_DEF_USB_HF_SEL);
			status = get_reg_param(fusb->otg_regs, FUSB_REG_OTG_UNLIMIT, FUSB_DEF_USB_HF_SEL);
		}

		switch(current_otg_mode) {
		case USB_OTG_CONTROL_HOST:
			if ((fusb->ehci_ops != NULL) && (fusb->ehci_ops->resume != NULL)) {
				fusb->ehci_ops->resume(fusb->ehci_ops->data);
			}

			if ((fusb->ohci_ops != NULL) && (fusb->ohci_ops->resume != NULL)) {
				fusb->ohci_ops->resume(fusb->ohci_ops->data);
			}
			break;
		case USB_OTG_CONTROL_GADGET:
			if ((fusb->gadget_ops != NULL) && (fusb->gadget_ops->resume != NULL)) {
				fusb->gadget_ops->resume(fusb->gadget_ops->data);
			}
			break;
		default:
			/* USB_OTG_CONTROL_STOP */
			/* USB_OTG_CONTROL_IDLE */
			break;
		}
		fusb->init_comp_flag = FUSB_OTG_FLAG_ON;
	}
	udif_mutex_unlock(&fusb->state_mutex);
	fusb_func_trace("END");
	return;
}
EXPORT_SYMBOL(fusb_otg_resume_initial_set);		/**<  */

void fusb_otg_host_enable_irq(unsigned int host, bool isEnable)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		fusb_func_trace("END1");
		return;
	}

	udif_mutex_lock(&fusb->state_mutex);

	switch (host) {
	case FUSB_OTG_HOST_EHCI:
		if ((fusb->ehci_ops != NULL) && (fusb->ehci_ops->enable_irq != NULL))
			fusb->ehci_ops->enable_irq(isEnable);
		break;
	case FUSB_OTG_HOST_OHCI:
		if ((fusb->ohci_ops != NULL) && (fusb->ohci_ops->enable_irq != NULL))
			fusb->ohci_ops->enable_irq(isEnable);
		break;
	default:
		break;
	}

	udif_mutex_unlock(&fusb->state_mutex);

	fusb_func_trace("END");

	return;
}
EXPORT_SYMBOL(fusb_otg_host_enable_irq);		/**<  */

void fusb_otg_host_resume_finish(unsigned int host)
{
	struct fusb_device_control *fusb;

	fusb_func_trace("START");

	fusb = g_fusb_device_control;
	if (fusb == NULL) {
		fusb_print_err("OTG Structure NULL pointer");
		fusb_func_trace("END1");
		return;
	}

	udif_mutex_lock(&fusb->state_mutex);

	fusb->host_resume_comp_flag |= host;

	if ((fusb->host_resume_comp_flag & FUSB_OTG_HOST_EHCI) &&
	    (fusb->host_resume_comp_flag & FUSB_OTG_HOST_OHCI)) {
		if ((fusb->ohci_ops != NULL) && (fusb->ohci_ops->enable_irq != NULL))
			fusb->ohci_ops->enable_irq(true);
		fusb->host_resume_comp_flag = 0;
	}

	udif_mutex_unlock(&fusb->state_mutex);

	fusb_func_trace("END");

	return;
}
EXPORT_SYMBOL(fusb_otg_host_resume_finish);		/**<  */

MODULE_LICENSE ("GPL"); 				/**<  */
