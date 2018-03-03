/*
 * usb_gadgetcore.h
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

#ifndef __USB_GADGETCORE_H__
#define __USB_GADGETCORE_H__

#ifdef  __KERNEL__

/***** struct *****/
// EPs
struct usb_gadget_ep_list {
    unsigned char num_ep;
    struct usb_ep **ep;
};

// GadgetFunctionDriver
struct usb_gadget_func_driver {
    char *function;
    unsigned char config;       /* Config Value (1-255) */
    unsigned char interface;    /* Interface Value (0-255) */
    void *context;
    unsigned char start_ext_info;
    
    void (*start)(struct usb_gadget_func_driver*, unsigned char, struct usb_gadget_ep_list);
    void (*stop)(struct usb_gadget_func_driver*);
    int (*ep_set_halt)(struct usb_gadget_func_driver*, struct usb_ep*);
    int (*ep_clear_halt)(struct usb_gadget_func_driver*, struct usb_ep*);
    void (*suspend)(struct usb_gadget_func_driver*);
    void (*resume)(struct usb_gadget_func_driver*);
    void (*busreset)(struct usb_gadget_func_driver*);
    int (*class)(struct usb_gadget_func_driver*, const struct usb_ctrlrequest*, struct usb_ep*);
    int (*vendor)(struct usb_gadget_func_driver*, const struct usb_ctrlrequest*, struct usb_ep*);
};

/***** typedef *****/

/***** macro *****/
// for start_ext_info
#define USB_GCORE_STARTEXT_NORMAL 0
#define USB_GCORE_STARTEXT_BYDUAL 1
// ep_set_halt() / ep_clear_halt() return value
#define USB_GADGETCORE_STALL        1
#define USB_GADGETCORE_THRU         2

// for special function driver alt num
#define USB_GADGETCORE_SPECIAL_FUNCDRV_ALT_NUM_POWER ( 0x01 )

/***** Proto Type *****/
// EXPORT_SYMBOL function
int usb_gadgetcore_register_driver(struct usb_gadget_func_driver*);
int usb_gadgetcore_unregister_driver(struct usb_gadget_func_driver*);
int usb_gadgetcore_stop_other_driver(struct usb_gadget_func_driver*);

#endif  /* __KERNEL__ */

struct usb_gadgetcore_probe_info;
struct usb_gadgetcore_start;
typedef struct _usb_gadget_desc_table usb_gadget_desc_table;

/******************** ioctl関連 ********************/

#define USB_IOC_GADGETCORE_BASE             (0xE2)

#define USB_IOC_GADGETCORE_PROBE  \
                _IOW(USB_IOC_GADGETCORE_BASE, 1, struct usb_gadgetcore_probe_info)
#define USB_IOC_GADGETCORE_REMOVE       _IO (USB_IOC_GADGETCORE_BASE, 2)
#define USB_IOC_GADGETCORE_START  \
                _IOW(USB_IOC_GADGETCORE_BASE, 3, struct usb_gadgetcore_start_info)
#define USB_IOC_GADGETCORE_STOP         _IO (USB_IOC_GADGETCORE_BASE, 4)


/******************** Event関連 ********************/
struct gadgetcore_event {
    void (*set_config)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*set_interface)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*testmode)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*bus_reset)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*suspend)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*resume)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
};

struct usb_gadgetcore_probe_info {
    usb_hndl_t hndl;                        /* ハンドラ */
    struct gadgetcore_event event;          /* イベントコールバック */
};

struct usb_gadgetcore_ext_start_info {
    bool enable_dual_mode;
    __u8 target_ep_adr;
    // other members may be added here.
};

struct usb_gadgetcore_start_info {
    usb_gadget_desc_table *desc_tbl;        /* Descriptor Table */
    void *serial_string;
    __u8 uc_serial_len;
    struct usb_gadgetcore_ext_start_info ext_info; /* Extended start info parameters */
};

// Event ID
enum {
    USB_KEVENT_ID_GADGETCORE_SET_CONFIG = 0,
    USB_KEVENT_ID_GADGETCORE_SET_INTERFACE,
    USB_KEVENT_ID_GADGETCORE_TESTMODE,
    USB_KEVENT_ID_GADGETCORE_BUS_RESET,
    USB_KEVENT_ID_GADGETCORE_SUSPEND,
    USB_KEVENT_ID_GADGETCORE_RESUME,
    USB_KEVENT_ID_GADGETCORE_NBROF,
};

// for USB_KEVENT_ID_GADGETCORE_SET_CONFIG
struct usb_kevent_arg_gadgetcore_set_config {
    unsigned char config;           /* Config値 */
};

// for USB_KEVENT_ID_GADGETCORE_SET_INTERFACE
struct usb_kevent_arg_gadgetcore_set_interface {
    unsigned char interface;        /* 対象のInterface番号 */
    unsigned char alt;              /* Alt値 */
};

// for USB_KEVENT_ID_GADGETCORE_TESTMODE
struct usb_kevent_arg_gadgetcore_testmode {
    unsigned char test_mode_selectors;      /* TestMode */
};

/******************** Descriptor関連 ********************/
// usb_gadget_if_table_element::uc_ep_restrictions
#define USB_GADGETCORE_UNUSE_DMA    (1 << 0)

typedef struct {
    __u8   uc_ep_address;
    __u8   uc_attributes;
    __u16  us_max_psize;
    __u8   uc_interval;
    __u8   uc_buffer_multiplex;
    __u8   uc_ep_restrictions;
    __u32  class_specific_ep_desc_size;   // Class定義のDescriptporのサイズ
    void   *class_specific_ep_desc;       // Class定義のDescriptpor
} usb_gadget_if_table_element;

typedef struct {
    __u8   uc_setting_number;                    // Alternate Setting
    __u8   uc_num_pep_list;
    usb_gadget_if_table_element *pep_list;
    __u8   uc_class;
    __u8   uc_sub_class;
    __u8   uc_interface_protocol; 
    __u8   uc_interface_str;
    __u32  class_specific_interface_desc_size;   // Class定義のDescriptporのサイズ
    void   *class_specific_interface_desc;       // Class定義のDescriptpor
} usb_gadget_if_table;

typedef struct {
    __u8     uc_if_number;       // interface index
    __u8     uc_num_if_tables;   // Alternateの数
    usb_gadget_if_table *if_tables;
} usb_gadget_interface_desc;

typedef struct {
    __u8 uc_firstinterface;
    __u8 uc_interface_count;
    __u8 uc_function_class;
    __u8 uc_function_sub_class;
    __u8 uc_function_protocol;
    __u8 uc_function;
} usb_gadget_ia_desc;

typedef struct {
    __u8 uc_attributes;
} usb_gadget_otg_desc;

typedef struct {
    __u8    uc_index;
    __u8    uc_configuration_value;
    __u8    uc_num_interfaces;
    usb_gadget_interface_desc *interfaces;
    __u8    uc_configuration_str;
    __u8    uc_attributes;
    __u8    uc_max_power;
    usb_gadget_otg_desc *otg;
    __u8    uc_num_iads;
    usb_gadget_ia_desc *iads;
} usb_gadget_config_desc;

typedef struct {
    __u16 us_bcd_usb;
    __u8  uc_class;
    __u8  uc_sub_class;
    __u8  uc_protocol;
    __u8  uc_num_config;
    usb_gadget_config_desc *configurations;
    __u8  uc_otg_attributes;
} usb_gadget_speed_desc;

typedef struct {
    __u8 uc_index;
    __u8 uc_len;
    void* string;
} usb_gadget_string_desc;

typedef struct {
    __u16  us_langid;
    __u8   uc_num_strings;
    usb_gadget_string_desc* strings;
} usb_gadget_langid_table;

typedef struct {
    __u16 us_desc_size;
    void* desc_data;
} usb_gadget_report_desc;

struct _usb_gadget_desc_table {
    usb_gadget_speed_desc hs;
    usb_gadget_speed_desc lsfs;
    __u16 us_id_vendor;
    __u16 us_id_product;
    __u16 us_bcd_device;
    __u8  uc_default_attributes;
    __u8  uc_manufacturer_str;
    __u8  uc_product_str;
    __u8  uc_serial_number_str;
    __u8  uc_num_langids;
    usb_gadget_langid_table* langids;
    usb_gadget_report_desc rptdesc;
};

#endif /* __USB_GADGETCORE_H__ */
