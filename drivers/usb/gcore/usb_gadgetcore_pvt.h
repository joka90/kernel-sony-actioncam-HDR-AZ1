/*
 * usb_gadgetcore_pvt.h
 * 
 * Copyright 2005,2006,2011 Sony Corporation
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
#ifndef __USB_GADGETCORE_PVT_H
#define __USB_GADGETCORE_PVT_H
/*-----------------------------------------------------------------------------
 * Include file
 *---------------------------------------------------------------------------*/
#include <linux/module.h>
#include <asm/types.h>
#include <linux/device.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
#include <linux/usb/gadget.h>
#else
#include <linux/usb_gadget.h>
#endif

#include <linux/usb/gcore/usb_event.h>
#include <linux/usb/gcore/usb_gadgetcore.h>

/*-----------------------------------------------------------------------------
 * struct declaration
 *---------------------------------------------------------------------------*/
struct alt_num {
    u8 interface_num;
    u8 alternate_num;
};

struct g_ep_info {
    u8 use;
    u8 interface_num;
    u8 ep_adr;
    u8 ep_halt;
};

struct g_core_ep {
    struct usb_ep ep;    /* FuncDrvへ渡すEP */
    struct usb_ep *_ep;  /* Controller管理のEP */
};

struct g_core_request {
    struct usb_request req;      /* FuncDrvへ渡すreq */
    struct usb_request *_req;    /* Controller管理のreq */
    
    struct g_core_ep *gep;
};

struct g_func_drv {
    struct usb_gadget_func_driver *func_drv;
    struct list_head list;
    
    struct usb_gadget_ep_list ep_list;
    unsigned char started;
};

struct g_core_drv {
    struct usb_gadget           *gadget;
    struct usb_ep               *ep0;
    struct usb_request          *ep0req;
    void                        (*ep0_comp_func)(struct g_core_drv *g_core,
                                                 struct usb_request*);
    struct  {
        u8 interface_num;
        u8 alt_num;
    } set_interface_info;
    
    struct usb_gadgetcore_probe_info g_probe;
    
    spinlock_t                  lock;
    spinlock_t                  lock_stop_config;
    unsigned long               atomic_bitflags;
#define USB_GCORE_ATOMIC_SUSPENDED         0
#define USB_GCORE_ATOMIC_FD_LOCK           1      /* FDアクセス排他処理用 */
#define USB_GCORE_ATOMIC_START             2      /* Start状態排他処理用 */


    usb_gadget_desc_table       *desc_tbl;
    struct alt_num              *alt_nums;
    
    u8                          set_config;
    
    struct list_head            func_list;
    
    u8                          hs_disable;
    u8                          test_mode;
    u8                          dev_feature;
    u8                          ep0_halt;
    u8                          enable_dual_mode;
    u8                          target_ep_adr;
};

/*-----------------------------------------------------------------------------
 * Macro declaration
 *---------------------------------------------------------------------------*/
/* for Debug */
/*
#ifdef CONFIG_USB_GADGET_CORE_LOGGING
# undef  CONFIG_USB_GADGET_CORE_LOGGING
# define CONFIG_USB_GADGET_CORE_LOGGING 5
#endif
*/
#ifndef CONFIG_USB_GADGET_CORE_LOGGING
#define CONFIG_USB_GADGET_CORE_LOGGING 0
#endif

#if CONFIG_USB_GADGET_CORE_LOGGING > 0
#define PDEBUG(fmt,args...)     printk(MYDRIVER_NAME "%04d: " fmt , __LINE__, ## args)
#else
#define PDEBUG(fmt,args...)     do { } while (0)
#endif

#if CONFIG_USB_GADGET_CORE_LOGGING > 1
#define PVERBOSE                PDEBUG
#else
#define PVERBOSE(fmt,args...)   do { } while (0)
#endif

#if CONFIG_USB_GADGET_CORE_LOGGING != 0
#define PWARN(fmt,args...)      printk(MYDRIVER_NAME "%04d: " fmt , __LINE__, ## args)
#define PINFO(fmt,args...)      printk(MYDRIVER_NAME "%04d: " fmt , __LINE__, ## args)
#else
#define PWARN(fmt,args...)      do { } while (0)
#define PINFO(fmt,args...)      do { } while (0)
#endif

#define PERR(fmt,args...)       printk(KERN_ERR MYDRIVER_NAME "%04d: " fmt , __LINE__, ## args)


#define     USB_GCORE_INT_NOT_DEF     255


/*-----------------------------------------------------------------------------
 * prototype
 *---------------------------------------------------------------------------*/
/* usb_gcore_main.c */
int start_testmode(struct g_core_drv*, u8);
int stop_testmode(struct g_core_drv*);

/* usb_gcore_setup.c */
int setup_req(struct g_core_drv*, const struct usb_ctrlrequest*);
void setup_disconnect(struct g_core_drv*);

/* usb_gcore_desc.c */
usb_gadget_config_desc* get_current_config_desc(struct g_core_drv*);
usb_gadget_config_desc* get_config_desc_from_config_num(struct g_core_drv*, u8);
usb_gadget_interface_desc* get_interface_desc(struct g_core_drv*, u8, u8);
usb_gadget_if_table* get_if_table_desc(struct g_core_drv*, u8, u8, u8);
u8 make_alt_num_list(struct g_core_drv*);
int set_alt_num(struct g_core_drv*, u8, u8);
int get_alt_num(struct g_core_drv*, u8, u8*);
void free_alt_num_list(struct g_core_drv*);
int save_desc_tbl(struct g_core_drv*, struct usb_gadgetcore_start_info*);
void free_desc_tbl(struct g_core_drv*);
void dump_device_desc(usb_gadget_desc_table*);
int is_hnp_capable(struct g_core_drv*);

int make_device_desc(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
int make_hid_desc(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
int make_report_desc(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
int make_device_qualifier_desc(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
int make_config_desc(struct g_core_drv*, const struct usb_ctrlrequest*, void*, u8);
int make_string_desc(struct g_core_drv*, const struct usb_ctrlrequest*, void*);

/* usb_gcore_ep.c */
struct usb_ep *ep_ctrl_enable_ep(struct g_core_drv*, u8, usb_gadget_if_table_element*);
int ep_ctrl_disable_ep(struct g_core_drv*, u8);
struct usb_ep *ep_ctrl_create_ep(struct g_core_drv*, struct usb_ep*);
int ep_ctrl_delete_ep(struct g_core_drv*, struct usb_ep*);
u8 ep_ctrl_get_in_num(struct g_core_drv *g_core, u8 ep_adr);
int delete_ep(struct g_core_drv*, struct usb_ep*);
int ep_ctrl_get_ep0_halt(struct g_core_drv*);
int ep_ctrl_get_ep_stall(struct g_core_drv*, u8);

/* usb_gcore_bus.c */
void bus_suspend(struct g_core_drv*);
void bus_resume(struct g_core_drv*);
void bus_disconnect(struct g_core_drv*);

#endif //#define __USB_GADGETCORE_PVT_H
