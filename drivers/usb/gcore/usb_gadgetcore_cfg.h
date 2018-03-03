/*
 * usb_gadgetcore_cfg.h
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

#ifndef __USB_GADGETCORE_CFG_H__
#define __USB_GADGETCORE_CFG_H__

/*-----------------------------------------------------------------------------
 * Driver configuration
 *---------------------------------------------------------------------------*/
#define EP0_BUFSIZE             256

// #define IGNORE_SAME_SET_CONFIG
// #define IGNORE_SAME_SET_INTERFACE

/*-----------------------------------------------------------------------------
 * Driver infomation
 *---------------------------------------------------------------------------*/
#define USBGADGETCORE_NAME      "usb_gadgetcore"
#define USBGADGETCORE_VERSION   "01.00.004"

#define USBGADGETCORE_MINOR_NUMBER  152

#define MYDRIVER_NAME           USBGADGETCORE_NAME

/*-----------------------------------------------------------------------------
 * Compile switch
 *---------------------------------------------------------------------------*/
#undef DESC_VERBOSE

#endif /* __USB_GADGETCORE_CFG_H__ */
