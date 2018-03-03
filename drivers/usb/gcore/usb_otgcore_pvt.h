/*
 * usb_otgcore_pvt.h
 * 
 * Copyright 2005,2006,2008,2011,2013 Sony Corporation
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

/*-----------------------------------------------------------------------------
 * Include file
 *---------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/usb/gcore/usb_event.h>
#include <linux/usb/gcore/usb_otgcore.h>

/*-----------------------------------------------------------------------------
 * struct declaration
 *---------------------------------------------------------------------------*/
struct usb_otgcore_port { /* ポート管理情報構造体*/
    int                         id;     /* ポート番号 */
    unsigned char               cid;    /* CIDの状態 */
    unsigned char               vbus;   /* VBUSの状態 */
};

struct otg_core_drv {
    struct usb_otg_control      *otg_control;
    struct usb_otgcore_probe_info otg_probe;
    struct usb_otgcore_port    *port;               /* ポート管理情報 */
    unsigned char               port_num;           /* ポート数 */
    unsigned char               test_mode;          /* TestModeの状態 */
    unsigned char               hs_disable;
    
    struct list_head            drv_list;
    
    spinlock_t                  lock;
    UDIF_MUTEX                  sem;
    unsigned long               bitflags;
#define USB_OTGCORE_ATOMIC_FD_LOCK           0      /* FDアクセス排他処理用 */
#define USB_OTGCORE_ATOMIC_BIND              1      /* Controllerからのbind済みフラグ */

#define USB_OTGCORE_ATOMIC_HNP_ENABLE_USER   2      /* Userによるhnp許可フラグ */
#define USB_OTGCORE_ATOMIC_HNP_ENABLE_HOST   3      /* Hostによるhnp許可フラグ */
#define USB_OTGCORE_ATOMIC_ENABLE_RCHOST     4      /* usb_otg_control_enable_rchost() 済み */

#define USB_OTGCORE_ATOMIC_E_HOST            5
};

struct m_otg_drv {
    struct usb_otg_driver *otg_drv;
    struct list_head list;
};

/*-----------------------------------------------------------------------------
 * Macro declaration
 *---------------------------------------------------------------------------*/
/* for Debug */
#ifndef CONFIG_USB_OTG_CORE_LOGGING
#define CONFIG_USB_OTG_CORE_LOGGING 0
#endif

#if CONFIG_USB_OTG_CORE_LOGGING > 0
#define PDEBUG(fmt,args...)     printk(MYDRIVER_NAME ": " fmt , ## args)
#else
#define PDEBUG(fmt,args...)     do { } while (0)
#endif

#if CONFIG_USB_OTG_CORE_LOGGING > 1
#define PVERBOSE                PDEBUG
#else
#define PVERBOSE(fmt,args...)   do { } while (0)
#endif

#if CONFIG_USB_OTG_CORE_LOGGING != 0
#define PWARN(fmt,args...)      printk(MYDRIVER_NAME ": " fmt , ## args)
#define PINFO(fmt,args...)      printk(MYDRIVER_NAME ": " fmt , ## args)
#else
#define PWARN(fmt,args...)      do { } while (0)
#define PINFO(fmt,args...)      do { } while (0)
#endif

#define PERR(fmt,args...)       printk(KERN_ERR MYDRIVER_NAME ": " fmt , ## args)

