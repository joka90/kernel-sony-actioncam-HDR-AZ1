/*
 * driver/usb/gcore/usb_init.c
 *
 * Serialized and ordered initcall for usb static kernel modules.
 *
 * Copyright 2011,2013 Sony Corporation
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
/*
 * Each static driver should care that his/her init()
 * is called from module_init() macro or this target_init().
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>

extern int f_usb20hdc_udc_init( void );
extern int usb_event_module_init(void);
extern int usb_otgcore_module_init(void);
extern int usb_gcore_module_init(void);

static int __init sony_usb_init(void)
{
	printk(KERN_INFO "%s: CXD4132\n", __func__);

	/*
	 * put the static moudle's init()s here.
	 */
#if defined(CONFIG_USB_FUSB_OTG)
	f_usb20hdc_udc_init();
#endif

#if defined(CONFIG_USB_EVENT)
	usb_event_module_init();
#endif

#if defined(CONFIG_USB_OTG_CORE)
	usb_otgcore_module_init();
#endif

#if defined(CONFIG_USB_GADGET_CORE)
	usb_gcore_module_init();
#endif

	return 0;
}

/* module_init() is too early. */
late_initcall(sony_usb_init);
