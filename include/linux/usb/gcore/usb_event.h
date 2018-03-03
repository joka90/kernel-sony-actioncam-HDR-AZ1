/*
 * usb_event.h
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

#ifndef __USB_EVENT_H__
#define __USB_EVENT_H__

/******************** 型定義 ********************/
typedef unsigned long usb_hndl_t;
typedef unsigned char usb_kevent_id_t;

#ifdef  __KERNEL__

/******************** 型定義 ********************/
typedef void (*usb_event_callback)(usb_hndl_t,
                                   usb_kevent_id_t,
                                   unsigned char,
                                   void*
                                   );

/******************** Proto Type ********************/
int usb_event_add_queue(unsigned char,
                        usb_event_callback,
                        usb_hndl_t,
                        usb_kevent_id_t,
                        unsigned char,
                        void*);

/******************** Macro ********************/
#define USB_EVENT_PRI_NORMAL    (0)

#endif  /* __KERNEL__ */

/******************** 設定値 ********************/
/* Queueのサイズ(初期値) */
#define USB_EVENT_DEFAULT_QUEUE_SIZE    4096

/* HandleListの数(初期値) */
#define USB_EVENT_DEFAULT_NUM_HDL_LIST    32


/******************** ioctl関連 ********************/
/* IOCTL用Command定義 */
#define USB_IOC_EVENT_BASE              0xE1

#define USB_IOC_EVENT_ENABLE            _IOW(USB_IOC_EVENT_BASE, 1, usb_hndl_t)
#define USB_IOC_EVENT_DISABLE           _IOW(USB_IOC_EVENT_BASE, 2, usb_hndl_t)
#define USB_IOC_EVENT_STOP              _IO (USB_IOC_EVENT_BASE, 3)


#endif /* __USB_EVENT_CMN_H__ */
