/*
 * usb_event_pvt.h
 * 
 * Copyright 2005,2006 Sony Corporation
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
#include <linux/usb/gcore/usb_event.h>

/*-----------------------------------------------------------------------------
 * Macro declaration
 *---------------------------------------------------------------------------*/
/* for Debug */
#ifndef CONFIG_USB_EVENT_LOGGING
#define CONFIG_USB_EVENT_LOGGING 0
#endif

#if CONFIG_USB_EVENT_LOGGING > 0
#define PDEBUG(fmt,args...)     printk(MYDRIVER_NAME ": " fmt , ## args)
#else
#define PDEBUG(fmt,args...)     do { } while (0)
#endif

#if CONFIG_USB_EVENT_LOGGING > 1
#define PVERBOSE                PDEBUG
#else
#define PVERBOSE(fmt,args...)   do { } while (0)
#endif

#if CONFIG_USB_EVENT_LOGGING != 0
#define PWARN(fmt,args...)      printk(MYDRIVER_NAME ": " fmt , ## args)
#define PINFO(fmt,args...)      printk(MYDRIVER_NAME ": " fmt , ## args)
#else
#define PWARN(fmt,args...)      do { } while (0)
#define PINFO(fmt,args...)      do { } while (0)
#endif

#define PERR(fmt,args...)       { printk(KERN_ERR MYDRIVER_NAME ": " fmt , ## args); }


/*-----------------------------------------------------------------------------
 * Macro declaration
 *---------------------------------------------------------------------------*/
/* アトミックデータ */
#define USB_EVENT_ATOMIC_FD_LOCK           0      /* FDアクセス排他処理用 */
#define USB_EVENT_ATOMIC_WAKEUP            1      /* 強制wakeup処理用 */

/* Queue操作用Macro */
#define GET_WRITE_P(q)          (((q)->read_p + (q)->data_size) % (q)->buf_size)
#define GET_DATA_SIZE(q)        ((q)->data_size)
#define SET_DATA_SIZE(q,size)   ((q)->data_size = size)
#define FORWARD_WRITE_P(q,size) ({(q)->data_size += (size); \
                                 (q)->max_data_size = max((q)->data_size, (q)->max_data_size);})
#define FORWARD_READ_P(q,size)  ({(q)->read_p = ((q)->read_p + (size)) % (q)->buf_size; \
                                 (q)->data_size -= (size);                 })

#define PADDING_SIZE(s,bs)      (((bs) - ((s) % (bs))) % (bs))

