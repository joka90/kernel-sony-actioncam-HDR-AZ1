/*
 * usb_gcore_wrapper_cfg.h
 *
 * Copyright 2011 Sony Corporation
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

#ifndef __USB_GCORE_WRAPPER_CFG_H
#define __USB_GCORE_WRAPPER_CFG_H
#include <asm/types.h>

// For Debug
#define EPADR_DUALMODE_BULKOUT_EPADR 0x02

// Defs for DualMode
typedef enum {
    GCW_FALSE=0,
    GCW_TRUE=1,
} GCW_BOOL;

#define AUTOMODE_CONFIGNUM 0x01

#define MSC_IF_CLASS    0x08
#define MSC_IF_SUBCLASS 0x06
#define MSC_IF_PROTOCOL 0x50
#define SIC_IF_CLASS    0x06
#define SIC_IF_SUBCLASS 0x01
#define SIC_IF_PROTOCOL 0x01

#define MTP_COMMAND_HEADER					0x00000020
#define MTP_COMMAND_ID						0x0001

// Common Defs for OSStringDescriptor and FeatureDescriptor
#define USB_FEATURE_DESCRIPTOR_CODE    0x30
#define USB_FEATURE_DESCRIPTOR_VERSION 0x0100

// Defs for OSStringDescriptors
#define USB_REQ_OS_STRING_DESC_BREQUESTEYPE 0x80
#define USB_REQ_OS_STRING_DESC_BREQUEST     0x06
#define USB_REQ_OS_STRING_DESC_WVALUE_H     0x03
#define USB_REQ_OS_STRING_DESC_WVALUE_L     0xEE
#define USB_REQ_OS_STRING_DESC_WINDEX       0x0000
#define USB_REQ_OS_STRING_DESC_WLENGTH_MIN  0x02
#define USB_REQ_OS_STRING_DESC_WLENGTH_MAX  0xFF

#define USB_RES_OS_STRING_DESC_BLENGTH         0x12
#define USB_RES_OS_STRING_DESC_BDESCRIPTORTYPE 0x03
#define USB_RES_OS_STRING_DESC_QWSIGNATURE     {'M','\0','S','\0','F','\0','T','\0','1','\0','0','\0','0','\0'}
#define USB_RES_OS_STRING_DESC_BMS_VCODE       USB_FEATURE_DESCRIPTOR_CODE
#define USB_RES_OS_STRING_DESC_BPAD            0x00

// Defs for FeatureDescriptors
#define USB_REQ_FEATURE_DESC_BREQUESTEYPE 0xC0
#define USB_REQ_FEATURE_DESC_BREQUEST     USB_FEATURE_DESCRIPTOR_CODE
#define USB_REQ_FEATURE_DESC_WVALUE_H     0x00
#define USB_REQ_FEATURE_DESC_WVALUE_L     0x00
#define USB_REQ_FEATURE_DESC_WINDEX       0x0004
#define USB_REQ_FEATURE_DESC_WLENGTH_MIN  0x02
#define USB_REQ_FEATURE_DESC_WLENGTH_MAX  0xFF

#define USB_RES_FEATURE_DESC_DWLENGTH     0x00000028
#define USB_RES_FEATURE_DESC_BCDVERSION   USB_FEATURE_DESCRIPTOR_VERSION
#define USB_RES_FEATURE_DESC_WINDEX       0x0004
#define USB_RES_FEATURE_DESC_BCOUNT       0x01
#define USB_RES_FEATURE_DESC_RESERVED     {0x00,0x00,0x00,0x00,0x00,0x00}

#define USB_RES_FEATURE_DESC_BFIRSTINTERFACENUM   0x00
#define USB_RES_FEATURE_DESC_BFIRSTINTERFACECOUNT 0x01
#define USB_RES_FEATURE_DESC_COMPATIBLEID         {'M', 'T', 'P', '\0','\0','\0','\0','\0'}
#define USB_RES_FEATURE_DESC_SUBCOMPATIBLEID      {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
#define USB_RES_FEATURE_DESC_RESERVED             {0x00,0x00,0x00,0x00,0x00,0x00}


extern struct usb_os_descriptor g_os_desc;
extern struct usb_extended_configuration_descriptor g_ext_conf_desc;

/* // defined in usb_gcore_wrapper.c
struct usb_os_descriptor g_os_desc = {
  USB_RES_OS_STRING_DESC_BLENGTH,
  USB_RES_OS_STRING_DESC_BDESCRIPTORTYPE,
  USB_RES_OS_STRING_DESC_QWSIGNATURE,
  USB_RES_OS_STRING_DESC_BMS_VCODE,
  USB_RES_OS_STRING_DESC_BPAD
  };

struct usb_extended_configuration_descriptor g_ext_conf_desc = {
  USB_RES_FEATURE_DESC_DWLENGTH,
  USB_RES_FEATURE_DESC_BCDVERSION,
  USB_RES_FEATURE_DESC_WINDEX,
  USB_RES_FEATURE_DESC_BCOUNT,
  USB_RES_FEATURE_DESC_RESERVED,
  USB_RES_FEATURE_DESC_BFIRSTINTERFACENUM,
  USB_RES_FEATURE_DESC_BFIRSTINTERFACECOUNT,
  USB_RES_FEATURE_DESC_COMPATIBLEID,
  USB_RES_FEATURE_DESC_SUBCOMPATIBLEID,
  USB_RES_FEATURE_DESC_RESERVED
  };
 */
#endif /* uSB_GCORE_WRAPPER_CFG_H */
