/*
 * usb_otgcore.h
 * 
 * Copyright 2005,2006,2008,2009,2013 Sony Corporation
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

#ifndef __USB_OTGCORE_H__
#define __USB_OTGCORE_H__

#ifdef __KERNEL__

/***** struct *****/
struct usb_otgcore_ops {
};

struct usb_otgcore {
    const struct usb_otgcore_ops *ops;
};

struct usb_otg_driver {
    char *function;
    void *context;
    
    int  (*bind)(struct usb_otg_driver* drv, struct usb_otgcore* otgcore);
    int  (*unbind)(struct usb_otg_driver* drv, struct usb_otgcore* otgcore);
    void (*notify)(struct usb_otg_driver* drv, int notify);
    void (*notify_with_param)(struct usb_otg_driver* drv, int notify, int* pParam);
    int  (*query_over_current)(struct usb_otg_driver* drv);
};

struct usb_otgcore_line_state {
    int dp;
    int dm;
};

enum {
    USB_OTGCORE_LINE_STATE_LOW  = 0,
    USB_OTGCORE_LINE_STATE_HIGH = 1,
};

/***** Event Type *****/
enum {
    USB_OTGCORE_START_HOST = 1,
    USB_OTGCORE_STOP_HOST,
    USB_OTGCORE_B_DISCONNECT,
    USB_OTGCORE_CID_A_TO_B,
    USB_OTGCORE_EVT_CID_A,
    USB_OTGCORE_EVT_CID_B,
    USB_OTGCORE_EVT_VBUS_ON,
    USB_OTGCORE_EVT_VBUS_OFF,
};

/******************** Proto Type ********************/
// EXPORT_SYMBOL function
int usb_otgcore_register_driver(struct usb_otg_driver *drv);
int usb_otgcore_unregister_driver(struct usb_otg_driver *drv);
unsigned char usb_otgcore_get_hs_disable(void);
int  usb_otgcore_set_test_mode(unsigned char test_mode);
int  usb_otgcore_gadget_set_feature(__le16 feature_selector);
int usb_otgcore_gadget_suspend(void);
void usb_otgcore_gadget_disconnect(void);
int usb_otgcore_get_line_state(struct usb_otgcore_line_state *ls);

enum {
    USB_OTGCORE_RES_START_RCHOST = 1,
    USB_OTGCORE_RES_SUSPEND,
};

#endif  /* __KERNEL__ */

/******************** ioctl関連 ********************/
struct usb_otgcore_probe_info;

#define USB_IOC_OTGCORE_BASE                0xE3

#define USB_IOC_OTGCORE_PROBE   \
                _IOW(USB_IOC_OTGCORE_BASE, 1, struct usb_otgcore_probe_info)
#define USB_IOC_OTGCORE_REMOVE              _IO (USB_IOC_OTGCORE_BASE, 2)
#define USB_IOC_OTGCORE_STOP                _IO (USB_IOC_OTGCORE_BASE, 3)
#define USB_IOC_OTGCORE_IDLE                _IO (USB_IOC_OTGCORE_BASE, 4)
#define USB_IOC_OTGCORE_START_GADGET        _IO (USB_IOC_OTGCORE_BASE, 5)
#define USB_IOC_OTGCORE_START_HOST          _IO (USB_IOC_OTGCORE_BASE, 6)
#define USB_IOC_OTGCORE_START_E_HOST        _IO (USB_IOC_OTGCORE_BASE, 7)
#define USB_IOC_OTGCORE_ENABLE_RCHOST       _IO (USB_IOC_OTGCORE_BASE, 8)
#define USB_IOC_OTGCORE_DISABLE_RCHOST      _IO (USB_IOC_OTGCORE_BASE, 9)
#define USB_IOC_OTGCORE_START_RCGADGET      _IO (USB_IOC_OTGCORE_BASE, 10)
#define USB_IOC_OTGCORE_GET_SPEED           _IOR(USB_IOC_OTGCORE_BASE, 11, unsigned char)
#define USB_IOC_OTGCORE_SET_SPEED           _IOW(USB_IOC_OTGCORE_BASE, 12, unsigned char)
#define USB_IOC_OTGCORE_SET_TEST_MODE       _IOW(USB_IOC_OTGCORE_BASE, 13, unsigned char)
#define USB_IOC_OTGCORE_SEND_SRP            _IO (USB_IOC_OTGCORE_BASE, 14)
#define USB_IOC_OTGCORE_SELECT_PORT         _IOW(USB_IOC_OTGCORE_BASE, 15, unsigned char)
#define USB_IOC_OTGCORE_GET_PORTINFO \
                _IOR(USB_IOC_OTGCORE_BASE, 16, struct usb_otgcore_port_info)
#define USB_IOC_OTGCORE_REGIST_PORT_DESCRIPTOR \
                _IOW(USB_IOC_OTGCORE_BASE, 17, struct usb_otgcore_port_descriptor)
#define USB_IOC_OTGCORE_UNREGIST_PORT_DESCRIPTOR _IO(USB_IOC_OTGCORE_BASE, 18)
#define USB_IOC_OTGCORE_SET_PHY_PARAM \
                _IOW(USB_IOC_OTGCORE_BASE, 19, struct usb_otgcore_phy_param)

// for USB_IOC_OTGCORE_GET_SPEED
enum {
    USB_OTGCORE_SPEED_LS = 1,
    USB_OTGCORE_SPEED_FS,
    USB_OTGCORE_SPEED_HS,
};

// for USB_IOC_OTGCORE_SET_SPEED
enum {
    USB_OTGCORE_HS_ENABLE = 1,
    USB_OTGCORE_HS_DISABLE,
};

// for USB_IOC_OTGCORE_SET_TEST_MODE
enum {
    USB_OTGCORE_TEST_MODE_NORMAL = 1,
    USB_OTGCORE_TEST_MODE_G_PACKET,
    USB_OTGCORE_TEST_MODE_G_K,
    USB_OTGCORE_TEST_MODE_G_J,
    USB_OTGCORE_TEST_MODE_G_SE0_NAK,
    USB_OTGCORE_TEST_MODE_G_FORCE_PACKET,
    USB_OTGCORE_TEST_MODE_H_PACKET,
    USB_OTGCORE_TEST_MODE_H_LS,
    USB_OTGCORE_TEST_MODE_H_FS,
    USB_OTGCORE_TEST_MODE_H_HS,
};


/******************** Event関連 ********************/
struct otgcore_event {
    void (*cid)(usb_hndl_t, usb_kevent_id_t, unsigned char, void*);
    void (*vbus)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*vbus_error)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*pullup)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*receive_srp)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*rchost)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*rcgadget)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
    void (*set_feature)(usb_hndl_t, usb_kevent_id_t, unsigned char, void* vp_arg);
};

struct usb_otgcore_probe_info {
    usb_hndl_t hndl;                        /* ハンドラ */
    struct otgcore_event event;             /* イベントコールバック */
};

// Event ID
enum {
    USB_KEVENT_ID_OTGCORE_CID = 0,
    USB_KEVENT_ID_OTGCORE_VBUS,
    USB_KEVENT_ID_OTGCORE_VBUS_ERROR,
    USB_KEVENT_ID_OTGCORE_PULLUP,
    USB_KEVENT_ID_OTGCORE_RECEIVE_SRP,
    USB_KEVENT_ID_OTGCORE_RCHOST,
    USB_KEVENT_ID_OTGCORE_RCGADGET,
    USB_KEVENT_ID_OTGCORE_SET_FEATURE,
    USB_KEVENT_ID_OTGCORE_NBROF,
};

// Port ID
enum { 
    USB_OTGCORE_PORT0 = 0,
    USB_OTGCORE_PORT1 = 1,
};

// Port num
enum { 
    USB_OTGCORE_PORT_NOSET  = 0,
    USB_OTGCORE_PORT_SINGLE = 1,
    USB_OTGCORE_PORT_DOUBLE = 2,
    USB_OTGCORE_PORT_MAX = 2,
};

struct usb_otgcore_port_info {
    unsigned int nr;            /* ポート数 */
    unsigned int current_port;  /* 選択ポート */
};

// for USB_KEVENT_ID_OTGCORE_CID
struct usb_kevent_arg_otgcore_cid {
    unsigned char value;        /* CID値 */
    unsigned int  port;         /* イベントが発生したポート番号 */
                                /* USB_OTGCORE_CID_A / USB_OTGCORE_CID_B */
};

enum usb_otgcore_io_type {
        USB_OTGCORE_IO_TYPE_HFIX,
        USB_OTGCORE_IO_TYPE_LFIX,
        USB_OTGCORE_IO_TYPE_MISC,
        USB_OTGCORE_IO_TYPE_GPIO,
        USB_OTGCORE_IO_TYPE_GPIOX,
};

struct usb_otgcore_io {
        enum usb_otgcore_io_type type;
        unsigned char gpio_port;
        unsigned int  gpio_bit;
};

struct usb_otgcore_port_descriptor {
        unsigned char port_num;     /* number of ports */
        struct {
                struct usb_otgcore_io cid;
                struct usb_otgcore_io vbus;
        } port[USB_OTGCORE_PORT_MAX];
        struct usb_otgcore_io select;
};


struct usb_otgcore_phy_param {
    unsigned char phy_data[16];
};

enum { 
    USB_OTGCORE_CID_A = 1,
    USB_OTGCORE_CID_B,
};

// for USB_KEVENT_ID_OTGCORE_VBUS
struct usb_kevent_arg_otgcore_vbus {
    unsigned char value;        /* VBUS値 */
    unsigned int  port;         /* イベントが発生したポート番号 */
                                /* USB_OTGCORE_VBUS_ON / USB_OTGCORE_VBUS_OFF */
};

enum {
    USB_OTGCORE_VBUS_ON = 1,
    USB_OTGCORE_VBUS_OFF,
    USB_OTGCORE_VBUS_UNKNOWN = 99,  /* OTG Core 内部用 */
};

// for USB_KEVENT_ID_OTGCORE_PULLUP
struct usb_kevent_arg_otgcore_pullup {
    unsigned char value;        /* Pullup状態 */
    unsigned int  port;         /* イベントが発生したポート番号 */
                                /* USB_OTGCORE_PULLUP_ON / USB_OTGCORE_PULLUP_OFF */
};

enum {
    USB_OTGCORE_PULLUP_ON = 1,
    USB_OTGCORE_PULLUP_OFF,
};

// for USB_KEVENT_ID_OTGCORE_RCxx
struct usb_kevent_arg_otgcore_rchost {
    unsigned char result;       /* 結果 */
                                /* USB_OTGCORE_SUCCESS / USB_OTGCORE_FAIL */
};

struct usb_kevent_arg_otgcore_rcgadget {
    unsigned char result;       /* 結果 */
                                /* USB_OTGCORE_SUCCESS / USB_OTGCORE_FAIL */
};

enum {
    USB_OTGCORE_SUCCESS = 1,
    USB_OTGCORE_FAIL,
};

// for USB_KEVENT_ID_OTGCORE_SET_FEATURE
struct usb_kevent_arg_otgcore_set_feature {
    unsigned char b_hnp_enable;
    unsigned char a_hnp_support;
    unsigned char a_alt_hnp_support;
};

enum {
    USB_OTGCORE_UNSET = 0,
    USB_OTGCORE_SET,
};

#endif /* __USB_OTGCORE_H__ */
