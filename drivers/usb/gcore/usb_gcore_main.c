/*
 * usb_gcore_main.c
 * 
 * Copyright 2005,2006,2011,2013 Sony Corporation
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <asm/types.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif

#include <linux/list.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
#include <linux/usb/gadget.h>
#else
#include <linux/usb_gadget.h>
#endif

#include <linux/udif/cdev.h>
#include <mach/udif/devno.h>

#if defined(CONFIG_ARCH_CXD4132BASED)
#   include <linux/usb/scd/scd_ioctl.h>
#elif defined(CONFIG_ARCH_CXD4120)
#   include <linux/usb/mcd/usb_mcd_ioctl.h>
#elif defined(CONFIG_ARCH_CXD90014BASED)
#   include <linux/usb/f_usb/f_usb20hdc_ioctl.h>
#else
#   error CONFIG_ARCH error
#endif

#include <linux/usb/gcore/usb_event.h>
#include <linux/usb/gcore/usb_gadgetcore.h>
#include <linux/usb/gcore/usb_otgcore.h>

#include "usb_gadgetcore_cfg.h"
#include "usb_gadgetcore_pvt.h"

#ifdef DBG_PREFIX
# undef  DBG_PREFIX
# define DBG_PREFIX "GCORE_MAIN"
#else
# define DBG_PREFIX "GCORE_MAIN"
#endif
#include "usb_gcore_wrapper_cfg.h"
#include "usb_gcore_wrapper_pvt.h"

/*-----------------------------------------------------------------------------
 * Module infomation
 *---------------------------------------------------------------------------*/
MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION(USBGADGETCORE_NAME
                   " driver ver " USBGADGETCORE_VERSION);
MODULE_LICENSE("GPL");

/*-----------------------------------------------------------------------------
 * Function prototype declaration
 *---------------------------------------------------------------------------*/
int __init usb_gcore_module_init(void);

static void __exit usb_gcore_module_exit(void);
static int __init g_core_alloc(void);
static void g_core_free(struct g_core_drv*);
static int ioctl_probe(void*);
static int ioctl_remove(void*);
static int ioctl_start(void*);
static int ioctl_stop(void*);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long usb_gadgetcore_ioctl(struct file*, unsigned int, unsigned long);
#else
static int usb_gadgetcore_ioctl(struct inode*, struct file*, unsigned int, unsigned long);
#endif
static int usb_gadgetcore_open(struct inode *inode, struct file*);
static int usb_gadgetcore_release(struct inode *inode, struct file*);

static int g_bind(struct usb_gadget*);
static void g_unbind(struct usb_gadget*);
static int g_setup(struct usb_gadget*, const struct usb_ctrlrequest*);
static void g_disconnect(struct usb_gadget*);
static void g_suspend(struct usb_gadget*);
static void g_resume(struct usb_gadget*);

/*-----------------------------------------------------------------------------
 * Variable declaration
 *---------------------------------------------------------------------------*/
static struct usb_gadget_driver g_core_driver;

static struct g_core_drv *the_g_core;

static struct usb_gadget_driver g_core_driver = 
{
    .speed      = USB_SPEED_HIGH,
    .function   = USBGADGETCORE_NAME,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
    // .bind is abolished.
#else
    .bind       = g_bind,
#endif
    .unbind     = g_unbind,
    .setup      = g_setup,
    .disconnect = g_disconnect,
    .suspend    = g_suspend,
    .resume     = g_resume,
    .driver     = {
        .name       = USBGADGETCORE_NAME,
    },
};

/*=============================================================================
 *
 * Main function body
 *
 *===========================================================================*/
/*-----------------------------------------------------------------------------
 * ioctl実体
 *---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int
ioctl_probe(void *arg)
{
    struct g_core_drv *g_core = the_g_core;
    struct usb_gadgetcore_probe_info tmp_info;
    unsigned long flags;
    unsigned long res;
    
    PDEBUG("%s call\n", __func__);
    
    if(!g_core){
        return -EINVAL;
    }
    
    /* User空間からtmpへコピー */
    res = copy_from_user(&tmp_info, arg, sizeof(tmp_info));
    if(res != 0){
        PWARN("error: probe failed\n");
        return -EINVAL;
    }
    
    /* 引数が正しいかを確認 */
    if(tmp_info.hndl == 0){
        PWARN("error: handle is 0\n");
        return -EINVAL;
    }
    
    spin_lock_irqsave(&g_core->lock, flags);
    
    /* すでにprobe済みか確認 */
    if(g_core->g_probe.hndl != 0){
        spin_unlock_irqrestore(&g_core->lock, flags);
        PWARN("error: probe failed\n");
        return -EBUSY;
    }
    
    /* tmpからコピー */
    memcpy(&g_core->g_probe, &tmp_info, sizeof(tmp_info));
    
    spin_unlock_irqrestore(&g_core->lock, flags);
    
    PDEBUG("g_probe.hndl  : %lu\n", g_core->g_probe.hndl);
    PDEBUG("g_probe.event : %lx\n",  (unsigned long)&g_core->g_probe.event);
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_remove(void *arg)
{
    struct g_core_drv *g_core = the_g_core;
    unsigned long flags;
    
    PDEBUG("%s call\n", __func__);
    
    if(!g_core){
        return -EINVAL;
    }
    
    /* Handleを0にする */
    spin_lock_irqsave(&g_core->lock, flags);
    g_core->g_probe.hndl = 0;
    spin_unlock_irqrestore(&g_core->lock, flags);
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_start(void *arg)
{
    struct g_core_drv *g_core = the_g_core;
    int res;
    
    PDEBUG("%s call\n", __func__);
    
    if(!g_core){
        return -EINVAL;
    }
    
    PDEBUG("&arg : 0x%08lx\n", (unsigned long)arg);
    
    /* すでにstart済みでないか確認 */
    if(test_and_set_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags) != 0){
        PWARN("start: Fail\n");
        return -EBUSY;
    }
#ifdef GADGET_REGISTER_ON_DEMAND
    /* Gadget Controller へ regist する */
    PDEBUG("usb_gadget_register_driver()\n");
    //res = usb_gadget_register_driver(&g_core_driver);
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
    res = usb_gcw_probe_driver(&g_core_driver, g_bind );
 #else
    res = usb_gcw_register_driver(&g_core_driver);
 #endif
    if(res != 0){
        PERR("  -->failed\n");
        clear_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags);
        return res;
    }
#endif    
    g_core->hs_disable = usb_otgcore_get_hs_disable();
    
    /* 受け取ったDescriptorTableを保存する */
    res = save_desc_tbl(g_core, (struct usb_gadgetcore_start_info*)arg);
    if(res != 0){
        PERR("Error : save_desc_tbl()\n");
        clear_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags);
        return res;
    }

    // Inform GadgetCoreWrapper to be DualMode or not
    usb_gcw_start( g_core->enable_dual_mode );

#ifdef DESC_VERBOSE
    dump_device_desc(g_core->desc_tbl);
#endif /* DESC_VERBOSE */
    
    /* PullUP ON */
    res = usb_gadget_connect(g_core->gadget);
    if(res != 0){
        PERR("Error : usb_gadget_connect()\n");
        clear_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags);
    }
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_stop(void *arg)
{
    struct g_core_drv *g_core = the_g_core;
    int res;
    
    PDEBUG("%s call\n", __func__);
    
    if(!g_core){
        usb_gcw_stop();
        return -EINVAL;
    }
    
    /* start済みフラグを確認、クリアする */
    if(test_and_clear_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags)){
        /* PullUP OFF */
        res = usb_gadget_disconnect(g_core->gadget);
        if(res != 0){
            PERR("Error : usb_gadget_disconnect()\n");
        }
        
        /* 動作中のFunctionDriverを終了する -> SetConfig(0)の処理 */
        setup_disconnect(g_core);
        
        // Inform GadgetCoreWrapper to stop(Changes the gcw_state)
        usb_gcw_stop();
        
        /* DesciptorTableを破棄する */
        PDEBUG("free_desc_tbl()\n");
        free_desc_tbl(g_core);

#ifdef GADGET_REGISTER_ON_DEMAND        
        /* Gadget Controller から unregist する */
        //res = usb_gadget_unregister_driver(&g_core_driver);
        res = usb_gcw_unregister_driver(&g_core_driver);
        if(res != 0){
            PERR("Error: Unregist from GadgetController\n");
        }
#endif
    }
    
    return 0;
}

/*-----------------------------------------------------------------------------
 * File Operations
 *---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long
usb_gadgetcore_ioctl( struct file *filp,
#else
static int
usb_gadgetcore_ioctl(struct inode *inode,
                      struct file *filp,
#endif
                      unsigned int cmd,
                      unsigned long arg)
{
    int res;
    
    switch(cmd){
      /*----------------------------------------*/
      case USB_IOC_GADGETCORE_PROBE:
        res = ioctl_probe((void*)arg);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_GADGETCORE_REMOVE:
        res = ioctl_remove((char*)arg);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_GADGETCORE_START:
        res = ioctl_start((void*)arg);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_GADGETCORE_STOP:
        res = ioctl_stop((char*)arg);
        break;
        
      /*----------------------------------------*/
      default:
        PWARN("usb_gadgetcore_ioctl(none) call\n");
        res = -ENOTTY;
    }
    
    return res;
}

/*-------------------------------------------------------------------------*/
static int
usb_gadgetcore_open(struct inode *inode, struct file *filp)
{
    struct g_core_drv *g_core = the_g_core;
    
    /* 他のprocessがopenしていないか確認 */
    if(test_and_set_bit(USB_GCORE_ATOMIC_FD_LOCK, &g_core->atomic_bitflags) != 0){
        PWARN("usb_gadgetcore_open() failed\n");
        return -EBUSY;
    }
    
    PDEBUG("usb_gadgetcore_open() success\n");
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
usb_gadgetcore_release(struct inode *inode, struct file *filp)
{
    struct g_core_drv *g_core = the_g_core;
    
    PDEBUG("usb_gadgetcore_release() success\n");
    
    /* 使用中を解除 */
    clear_bit(USB_GCORE_ATOMIC_FD_LOCK, &g_core->atomic_bitflags);
    
    return 0;
}

/*-----------------------------------------------------------------------------
 * File Operations 構造体
 *---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static struct file_operations usb_gadgetcore_fops = 
{
    .owner  = THIS_MODULE,
    .open   = usb_gadgetcore_open,
    .release = usb_gadgetcore_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .unlocked_ioctl  = usb_gadgetcore_ioctl,
#else
    .ioctl  = usb_gadgetcore_ioctl,
#endif
};

struct cdev usb_gadgetcore_device;

/*-----------------------------------------------------------------------------
 * ControllerからのCallback関数
 *---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int
g_bind(struct usb_gadget *gadget)
{
    struct g_core_drv *g_core = the_g_core;
    struct g_ep_info        *ep_info;
    struct usb_ep           *ep;
    int rc = 0;
    
    PDEBUG("g_bind\n");
    PDEBUG("g_gadget : 0x%lx\n", (unsigned long)g_core);
    
    /* Controller から受け取った gadget構造体を保存 */
    g_core->gadget = gadget;
    
    set_gadget_data(gadget, g_core);
    g_core->ep0 = gadget->ep0;
    g_core->ep0->driver_data = g_core;
    
    /* EP0 用の request オブジェクトを取得 */
    g_core->ep0req = usb_ep_alloc_request(g_core->ep0, GFP_KERNEL);
    if(!g_core->ep0req){
        PDEBUG("usb_ep_alloc_request(EP0) ---> Fail\n");
        rc = -ENOMEM;
        goto exit;
    }
    PDEBUG("usb_ep_alloc_request(EP0) ---> Success\n");
    
    /* EP0 用の buffer を取得 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
    g_core->ep0req->buf = kmalloc(EP0_BUFSIZE, GFP_KERNEL);
    DEBUG_NEW(g_core->ep0req->buf);
#else
    g_core->ep0req->buf = usb_ep_alloc_buffer(g_core->ep0, EP0_BUFSIZE, NULL, GFP_KERNEL);
#endif
    if(!g_core->ep0req->buf){
        PDEBUG("buf alloc(EP0) ---> Fail\n");
        rc = -ENOMEM;
        usb_ep_free_request(g_core->ep0, g_core->ep0req);
        goto exit;
    }
    PDEBUG("buf alloc(EP0) ---> Success\n");
    
    /* EP リストを初期化 */
    list_for_each_entry(ep, &gadget->ep_list, ep_list){
        ep_info = kmalloc(sizeof(*ep_info), GFP_KERNEL);
        DEBUG_NEW(ep_info);
        PDEBUG(" ep_info: 0x%lx  cep: 0x%lx\n", (unsigned long)ep_info, (unsigned long)ep);
        if(!ep_info){
            rc = -ENOMEM;
        }else{
            ep_info->use = 0;
        }
        ep->driver_data = ep_info;
    }
    
    /* EP リストの初期化に失敗したときの処理 */
    if(rc != 0){
        PDEBUG("alloc ep_info ---> Fail\n");
        
        /* EP リストを開放 */
        list_for_each_entry(ep, &gadget->ep_list, ep_list){
            ep_info = ep->driver_data;
            if(!ep_info){
                continue;
            }
            PDEBUG("kfree ep_info: 0x%lx  cep: 0x%lx\n", (unsigned long)ep_info, (unsigned long)ep);
            DEBUG_FREE(ep_info);
            kfree(ep_info);
        }
        
        /* EP0のリソースを開放 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
        DEBUG_FREE(g_core->ep0req->buf);
        kfree(g_core->ep0req->buf);
#else
        usb_ep_free_buffer(g_core->ep0, g_core->ep0req->buf, (dma_addr_t)NULL, EP0_BUFSIZE);
#endif
        usb_ep_free_request(g_core->ep0, g_core->ep0req);
    }
    
exit:
    return rc;
}

/*-------------------------------------------------------------------------*/
static void
g_unbind(struct usb_gadget *gadget)
{
    struct g_core_drv  *g_core = get_gadget_data(gadget);
    struct usb_request *req = g_core->ep0req;
    struct g_ep_info        *ep_info;
    struct usb_ep           *ep;
    
    PDEBUG("g_unbind\n");
    
    /* EP0のリソースを開放 */
    if(req){
        if(req->buf){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
            DEBUG_FREE(req->buf);
            kfree(req->buf);
#else
            usb_ep_free_buffer(g_core->ep0, req->buf, req->dma, EP0_BUFSIZE);
#endif
        }
        usb_ep_free_request(g_core->ep0, req);
    }
    
    /* EP リストを開放 */
    list_for_each_entry(ep, &gadget->ep_list, ep_list){
        ep_info = ep->driver_data;
        PDEBUG("kfree ep_info: 0x%lx  cep: 0x%lx\n", (unsigned long)ep_info, (unsigned long)ep);
        if(!ep_info){
            continue;
        }
        DEBUG_FREE(ep_info);
        kfree(ep_info);
    }
    
    set_gadget_data(gadget, NULL);
    
    return;
}

/*-------------------------------------------------------------------------*/
static int
g_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
    struct g_core_drv  *g_core = get_gadget_data(gadget);
    int rc;
    
    PDEBUG("setup() %02x.%02x v%04x i%04x\n",
              ctrl->bRequestType, ctrl->bRequest, ctrl->wValue, ctrl->wIndex);
    
    /* start済みフラグを確認する */
    if(test_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags)){
        /* setup処理にまわす */
        rc = setup_req(g_core, ctrl);
    }else{
        rc = 0;
        PDEBUG("g_setup() in stop state\n");
    }
    
    return rc;
}

/*-------------------------------------------------------------------------*/
static void g_disconnect(struct usb_gadget *gadget)
{
    struct g_core_drv  *g_core = get_gadget_data(gadget);
    
    PDEBUG("g_disconnect() <==\n");
    
    /* TestModeを終了させる */
    if(g_core->test_mode){
        stop_testmode(g_core);
        g_core->test_mode = 0;
    }
    
    /* start済みフラグを確認する */
    if(test_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags)){
        bus_disconnect(g_core);
    }else{
        PDEBUG("g_disconnect() in stop state\n");
    }
    
    PDEBUG("==>\n");
}

/*-------------------------------------------------------------------------*/
static void g_suspend(struct usb_gadget *gadget)
{
    struct g_core_drv  *g_core = get_gadget_data(gadget);
    
    PDEBUG("g_suspend() <==\n");
    
    set_bit(USB_GCORE_ATOMIC_SUSPENDED, &g_core->atomic_bitflags);
    
    /* start済みフラグを確認する */
    if(test_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags)){
        /* Busモジュールへ通知 */
        bus_suspend(g_core);
    }else{
        PDEBUG("g_suspend() in stop state\n");
    }
    
    PDEBUG("==>\n");
}

/*-------------------------------------------------------------------------*/
static void g_resume(struct usb_gadget *gadget)
{
    struct g_core_drv  *g_core = get_gadget_data(gadget);
    
    PDEBUG("g_resume()  <==\n");
    
    clear_bit(USB_GCORE_ATOMIC_SUSPENDED, &g_core->atomic_bitflags);
    
    /* start済みフラグを確認する */
    if(test_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags)){
        /* Busモジュールへ通知 */
        bus_resume(g_core);
    }else{
        PDEBUG("g_resume() in stop state\n");
    }
    
    PDEBUG("==>\n");
}

/*-----------------------------------------------------------------------------
 * module operation
 *---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int __init
g_core_alloc(void)
{
    struct g_core_drv  *g_core;
    
    /* g_core 用の領域を取得 */
    g_core = kmalloc(sizeof *g_core, GFP_KERNEL);
    if(!g_core) return -ENOMEM;
    
    /* g_core を初期化 */
    memset(g_core, 0, sizeof *g_core);
    
    /* spin_lock用変数を初期化 */
    spin_lock_init(&g_core->lock);
    spin_lock_init(&g_core->lock_stop_config);
    
    the_g_core = g_core;
    
    PDEBUG("the_g_core : 0x%lx\n", (unsigned long)the_g_core);
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static void
g_core_free(struct g_core_drv *g_core)
{
    if(g_core != NULL){
        kfree(g_core);
    }
}

/*-------------------------------------------------------------------------*/

int __init usb_gcore_module_init(void)
{
    UDIF_DEVNODE *node;
    UDIF_DEVNO devno;
    struct g_core_drv *g_core;
    int res;
    
    PINFO("USB GadgetCore driver ver " USBGADGETCORE_VERSION "\n");
    
    /* g_core_drv 用の領域を確保 */
    res = g_core_alloc();
    if(res != 0){
        return res;
    }
    
    /* Gadget Core Driver 実体を取得 */
    g_core = the_g_core;
    
    /* FunctionDriverList を初期化する */
    INIT_LIST_HEAD(&g_core->func_list);

#ifndef GADGET_REGISTER_ON_DEMAND
    /* Gadget Controller へ regist する */
    PDEBUG("usb_gadget_register_driver()\n");
    //res = usb_gadget_register_driver(&g_core_driver);
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
    res = usb_gcw_probe_driver(&g_core_driver, g_bind); //MTP用
 #else
    res = usb_gcw_register_driver(&g_core_driver); //MTP用
 #endif
    if(res != 0){
        PERR("usb_gadget_register_driver()-->failed\n");
    }
#endif    
    
    // CharacterDeviceの登録
    node = udif_device_node(UDIF_NODE_USB_GCORE);
    devno = UDIF_MKDEV(node->major, node->first_minor);
    
    res = register_chrdev_region(devno, node->nr_minor, node->name);
    if(res != 0){
        goto exit;
    }
    
    cdev_init(&usb_gadgetcore_device, &usb_gadgetcore_fops);
    
    res = cdev_add(&usb_gadgetcore_device, devno, node->nr_minor);
    if(res != 0){
        unregister_chrdev_region(devno, node->nr_minor);
    }

exit:


    clear_bit(USB_GCORE_ATOMIC_FD_LOCK, &g_core->atomic_bitflags);
    
    if(res == 0){
        PDEBUG("usb_gcore_module_init() success\n");
    }else{
        printk(" -->fail!!\n");
        PDEBUG("usb_gcore_module_init() failed\n");
    }
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static
void __exit usb_gcore_module_exit(void)
{
    UDIF_DEVNODE *node;
    UDIF_DEVNO devno;
    int res;
    struct g_core_drv *g_core = the_g_core;
    
    // CharacterDeviceの登録削除
    node = udif_device_node(UDIF_NODE_USB_GCORE);
    devno = UDIF_MKDEV(node->major, node->first_minor);
    
    cdev_del(&usb_gadgetcore_device);
    unregister_chrdev_region(devno, node->nr_minor);
    
    /* start済みフラグを確認、クリアする */
    if(test_and_clear_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags)){
        /* PullUP OFF */
        res = usb_gadget_disconnect(g_core->gadget);
        if(res != 0){
            PERR("Error : usb_gadget_disconnect()\n");
        }
        
        /* 動作中のFunctionDriverを終了する -> SetConfig(0)の処理 */
        setup_disconnect(g_core);
        
        /* DesciptorTableを破棄する */
        PDEBUG("free_desc_tbl()\n");
        free_desc_tbl(g_core);
#ifdef GADGET_REGISTER_ON_DEMAND
        /* Gadget Controller から unregist する */
        //res = usb_gadget_unregister_driver(&g_core_driver);
        res = usb_gcw_unregister_driver(&g_core_driver);
        if(res != 0){
            PERR("Error: Unregist from GadgetController\n");
        }
#endif
    }
#ifndef GADGET_REGISTER_ON_DEMAND
    /* Gadget Controller から unregist する */
    //res = usb_gadget_unregister_driver(&g_core_driver);
    res = usb_gcw_unregister_driver(&g_core_driver);
    if(res != 0){
        PERR("Error: Unregist from GadgetController\n");
    }
#endif

    
    /* g_core構造体を開放 */
    g_core_free(g_core);
}

/*-----------------------------------------------------------------------------
 * TestMode
 *---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int start_testmode(struct g_core_drv *g_core, u8 testmode)
{
    int rc;
    
    switch(testmode){
      case 1:
        PDEBUG("TestMode: TEST_J\n");
        rc = usb_gadget_ioctl(g_core->gadget, 
                              USB_IOCTL_TESTMODE, 
                              USB_IOCTL_TESTMODE_TEST_J);
        break;
      case 2:
        PDEBUG("TestMode: TEST_K\n");
        rc = usb_gadget_ioctl(g_core->gadget, 
                              USB_IOCTL_TESTMODE, 
                              USB_IOCTL_TESTMODE_TEST_K);
        break;
      case 3:
        PDEBUG("TestMode: SE0_NAK\n");
        rc = usb_gadget_ioctl(g_core->gadget, 
                              USB_IOCTL_TESTMODE, 
                              USB_IOCTL_TESTMODE_SE0_NAK);
        break;
      case 4:
        PDEBUG("TestMode: TEST_PACKET\n");
        rc = usb_gadget_ioctl(g_core->gadget, 
                              USB_IOCTL_TESTMODE, 
                              USB_IOCTL_TESTMODE_TEST_PACKET);
        break;
      default:
        PWARN("TestMode: Error\n");
        rc = -EINVAL;
        break;
    }
    
    return rc;
}

/*-------------------------------------------------------------------------*/
int stop_testmode(struct g_core_drv *g_core)
{
    int rc;
    
    PDEBUG("TestMode: Stop!!\n");
    rc = usb_gadget_ioctl(g_core->gadget, 
                          USB_IOCTL_TESTMODE,
                          USB_IOCTL_TESTMODE_NORMAL);
    return rc;
}

/*-----------------------------------------------------------------------------
 * EXPORT_SYMBOL function
 *---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int
usb_gadgetcore_register_driver(struct usb_gadget_func_driver *func_drv)
{
    struct g_core_drv *g_core = the_g_core;
    struct g_func_drv *g_func_drv, *tmp_func_drv;
    unsigned long flags;
    
    PDEBUG("%s call\n", __func__);
    
    /* GadgetCoreがstart済みでないか確認 */
    if(test_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags) != 0){
        PWARN("usb_gadgetcore_register_driver(): Fail\n");
        return -EBUSY;
    }
    
    /* 必要なメンバが定義されているか確認 */
    if(!func_drv        ||
       !func_drv->start ||
       !func_drv->stop     ){
        return -EINVAL;
    }
    
    PDEBUG("========================================================\n");
    PDEBUG("FunctionDriver(%s) Added\n", func_drv->function ? func_drv->function : "");
    PDEBUG(" : 0x%lx\n", (unsigned long)func_drv);
    PDEBUG(" Config: %d Interface: %d\n", func_drv->config, func_drv->interface);
    PDEBUG(" start():         0x%lx\n", (unsigned long)func_drv->start);
    PDEBUG(" stop():          0x%lx\n", (unsigned long)func_drv->stop);
    PDEBUG(" ep_set_halt():   0x%lx\n", (unsigned long)func_drv->ep_set_halt);
    PDEBUG(" ep_clear_halt(): 0x%lx\n", (unsigned long)func_drv->ep_clear_halt);
    PDEBUG(" suspend():       0x%lx\n", (unsigned long)func_drv->suspend);
    PDEBUG(" resume():        0x%lx\n", (unsigned long)func_drv->resume);
    PDEBUG(" class():         0x%lx\n", (unsigned long)func_drv->class);
    PDEBUG(" vendor():        0x%lx\n", (unsigned long)func_drv->vendor);
    PDEBUG("========================================================\n");
    
    g_func_drv = (struct g_func_drv*)kmalloc(sizeof(*g_func_drv), GFP_KERNEL);
    DEBUG_NEW(g_func_drv);
    if(!g_func_drv){
        PWARN("kmalloc(): Fail\n");
        return -EINVAL;
    }
    
    PDEBUG("g_func_drv: 0x%lx\n", (unsigned long)g_func_drv);
    g_func_drv->func_drv = func_drv;
    g_func_drv->started = 0;
    
    spin_lock_irqsave(&g_core->lock, flags);
    
    /* FunctionDriverがリストに無いことを確認する */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv == g_func_drv->func_drv){
            /* すでに指定のFunctionDriverが登録されていたら エラーを返す */
            spin_unlock_irqrestore(&g_core->lock, flags);
            DEBUG_FREE(g_func_drv);
            kfree(g_func_drv);
            return -EINVAL;
        }
    }
    
    /* FunctionDriverをリストに追加 */
    list_add_tail(&g_func_drv->list, &g_core->func_list);
    
    spin_unlock_irqrestore(&g_core->lock, flags);
    
    /* FunctionDriverを表示 */
    PDEBUG("Print Func Driver :\n");
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        PDEBUG("     --> Cfg: %d In: %d %s\n", tmp_func_drv->func_drv->config,
                                               tmp_func_drv->func_drv->interface,
                                               tmp_func_drv->func_drv->function ? 
                                               tmp_func_drv->func_drv->function : "");
    }
    
    return 0;
}

/*-------------------------------------------------------------------------*/
int
usb_gadgetcore_unregister_driver(struct usb_gadget_func_driver *func_drv)
{
    struct g_core_drv *g_core = the_g_core;
    struct g_func_drv *tmp_func_drv, *n;
    unsigned long flags;
    
    PDEBUG("%s call\n", __func__);
    
    /* GadgetCoreがstart済みでないか確認 */
    if(test_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags) != 0){
        PWARN("usb_gadgetcore_unregister_driver(): Fail\n");
        return -EBUSY;
    }
    
    spin_lock_irqsave(&g_core->lock, flags);
    
    /* リストから指定のFunctionDriverを探す */
    list_for_each_entry_safe(tmp_func_drv, n, &g_core->func_list, list){
        if(tmp_func_drv->func_drv == func_drv){
            /* 指定のFunctionDriverが見つかったらリストから削除 */
            PDEBUG("del func_drv: 0x%lx\n", (unsigned long)tmp_func_drv->func_drv);
            PDEBUG("tmp_func_drv: 0x%lx\n", (unsigned long)tmp_func_drv);
            list_del(&tmp_func_drv->list);
            DEBUG_FREE(tmp_func_drv);
            kfree(tmp_func_drv);
        }
    }
    
    spin_unlock_irqrestore(&g_core->lock, flags);
    
    return 0;
}

/*-------------------------------------------------------------------------*/
int
usb_gadgetcore_stop_other_driver(struct usb_gadget_func_driver *func_drv)
{
    struct g_core_drv *g_core = the_g_core;
    struct g_func_drv *tmp_func_drv, *n;
    struct usb_ep **eps = NULL;
    unsigned long flags;
    u8 i;
    
    DEBUG_PRINT("%s", __FUNCTION__);
    PDEBUG("%s(%lx)\n", __func__, (unsigned long)func_drv);
    
    /* GadgetCoreがstart済みでないならば正常終了 */
    if(test_bit(USB_GCORE_ATOMIC_START, &g_core->atomic_bitflags) == 0){
        PWARN("usb_gadgetcore_stop_other_driver(): Fail\n");
        return 0;
    }
    
    /* スピンロック */
    spin_lock_irqsave(&g_core->lock, flags);
    
    /* リストから指定のFunctionDriver以外を探す */
    list_for_each_entry_safe(tmp_func_drv, n, &g_core->func_list, list){
        if(tmp_func_drv->func_drv != func_drv   &&
           tmp_func_drv->started != 0                 ){
            /* ここで条件の一致するFuncDrvへstop()通知を送る */
            PVERBOSE(" stop() to %s\n", 
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");
            DEBUG_PRINT("stop() to \"%s\"", 
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");
            tmp_func_drv->started = 0;
            tmp_func_drv->func_drv->stop(tmp_func_drv->func_drv);
            
            eps = tmp_func_drv->ep_list.ep;
            
            /* FuncDrv用EPリストを削除 */
            for(i = 0; i < tmp_func_drv->ep_list.num_ep; i++){
                ep_ctrl_delete_ep(g_core, *(eps+i));
            }
            DEBUG_FREE(eps);
            kfree(eps);
            
        }else{
            PVERBOSE(" skip: %s\n", 
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");
        }
    }
    
    /* スピンロック解除 */
    spin_unlock_irqrestore(&g_core->lock, flags);
    
    return 0;
}

struct g_func_drv *
findFuncDrv_fromIfClasses( u8 in_class, u8 in_sub_class, u8 in_protocol )
{
    struct g_func_drv *tmp_func_drv;
    struct g_func_drv *ret_func_drv=NULL;
    usb_gadget_if_table *if_table;
    struct g_core_drv *g_core = the_g_core;
    unsigned long flags;

    DEBUG_INFO("%s", __FUNCTION__);

    // 登録されたFuncDriverの中から探す
    DEBUG_INFO("checking funcDrivers with class=0x%x, sub_class=0x%x, protocol=0x%x",
               in_class, in_sub_class, in_protocol );

    /* -- Lock here -- */
    spin_lock_irqsave(&g_core->lock, flags);

    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        DEBUG_INFO("+ \"%s\" : config = 0x%x, interface = 0x%x",
                   tmp_func_drv->func_drv->function,
                   tmp_func_drv->func_drv->config,
                   tmp_func_drv->func_drv->interface);

        // alt_numは現在のalt_numを使用する
        if_table = get_if_table_desc( g_core, tmp_func_drv->func_drv->config, tmp_func_drv->func_drv->interface, g_core->set_interface_info.alt_num );
        DEBUG_INFO(" =>ifTbl =%p: class = 0x%x, sub_class = 0x%x, protocol = 0x%x, alt_num = 0x%x",
                   if_table,
                   if_table->uc_class,
                   if_table->uc_sub_class,
                   if_table->uc_interface_protocol,
                   g_core->set_interface_info.alt_num );

        // 3つの要素を比較する
        if( if_table && ( if_table->uc_class                 == in_class )
            && ( if_table->uc_sub_class          == in_sub_class )
            && ( if_table->uc_interface_protocol == in_protocol ) ){
            DEBUG_INFO("found!! \"%s\"", tmp_func_drv->func_drv->function);
            ret_func_drv = tmp_func_drv;
            goto exit;
        }
        else{
            PVERBOSE(" skip: %s\n", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
        }
    }

  exit:

    /* -- Unlock Here -- */
    spin_unlock_irqrestore(&g_core->lock, flags);
    return ret_func_drv;
}

struct usb_gadget_ep_list stopFuncDrv_fromIfClasses( u8 in_class, u8 in_sub_class, u8 in_protocol )
{
    struct g_func_drv *tmp_func_drv;
    usb_gadget_if_table *if_table;
    struct g_core_drv *g_core = the_g_core;
    struct usb_gadget_ep_list ret_ep_list;
    int first_stopped=0;

    struct usb_ep **feps = NULL;
    int i;

    unsigned long flags;

    DEBUG_PRINT("%s", __FUNCTION__);
    
    // Initialize ret_ep_list
    ret_ep_list.num_ep = 0; 
    ret_ep_list.ep = NULL; 

    // 登録されたFuncDriverの中から探す
    DEBUG_INFO("checking funcDrivers with class=0x%x, sub_class=0x%x, protocol=0x%x",
               in_class, in_sub_class, in_protocol );

    /* -- Lock here -- */
    spin_lock_irqsave(&g_core->lock, flags);

    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        DEBUG_INFO("+ \"%s\" : config = 0x%x, interface = 0x%x",
                   tmp_func_drv->func_drv->function,
                   tmp_func_drv->func_drv->config,
                   tmp_func_drv->func_drv->interface);

        // alt_numは現在のalt_numを使用する
        if_table = get_if_table_desc( g_core, tmp_func_drv->func_drv->config, tmp_func_drv->func_drv->interface, g_core->set_interface_info.alt_num );
        DEBUG_INFO(" =>ifTbl =%p: class = 0x%x, sub_class = 0x%x, protocol = 0x%x, alt_num = 0x%x",
                   if_table,
                   if_table->uc_class,
                   if_table->uc_sub_class,
                   if_table->uc_interface_protocol,
                   g_core->set_interface_info.alt_num );

        // 3つの要素を比較する
        if( if_table && ( tmp_func_drv->started != 0 )
            && ( if_table->uc_class              == in_class )
            && ( if_table->uc_sub_class          == in_sub_class )
            && ( if_table->uc_interface_protocol == in_protocol ) ){
            DEBUG_PRINT("found!! STOPPING \"%s\"", tmp_func_drv->func_drv->function);
            /* ここで条件の一致するFuncDrvへstop()通知を送る */
            PVERBOSE(" stop() to %s\n",
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");
            tmp_func_drv->started = 0;

            tmp_func_drv->func_drv->stop(tmp_func_drv->func_drv);
            DEBUG_PRINT(" stopped : %s", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");

            if( first_stopped == 0 ){
                memcpy( &ret_ep_list, &tmp_func_drv->ep_list, sizeof(ret_ep_list) );
                DEBUG_PRINT("copied ep_list info. num=%d, ep=%p", ret_ep_list.num_ep, ret_ep_list.ep);
                DEBUG_PRINT("these ep will be passed to SIC funcDrv");
                first_stopped = 1;
            }
            else{
                /* FuncDrv用EPリストを削除 */
                feps = tmp_func_drv->ep_list.ep;
                DEBUG_PRINT("not the first stop for this funcDrv. free them");
                for(i = 0; i < tmp_func_drv->ep_list.num_ep; i++){
                    ep_ctrl_delete_ep(g_core, *(feps+i));
                }
                if(feps != NULL){
                    DEBUG_FREE(feps);
                    kfree(feps);
                    tmp_func_drv->ep_list.ep = NULL;
                }
            }
        }

        else{
            DEBUG_PRINT("skipped : %s", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
            PVERBOSE(" skip: %s\n", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
        }
    }

    /* -- Unlock Here -- */
    spin_unlock_irqrestore(&g_core->lock, flags);

    return ret_ep_list;
}
int startFuncDrv_fromIfClasses( u8 in_class, u8 in_sub_class, u8 in_protocol, struct usb_gadget_ep_list ep_list, unsigned char ext_info )
{
    struct g_func_drv *tmp_func_drv;
    usb_gadget_if_table *if_table;
    struct g_core_drv *g_core = the_g_core;
    unsigned long flags;

    DEBUG_PRINT("%s", __FUNCTION__);

    // 登録されたFuncDriverの中から探す
    DEBUG_INFO("checking funcDrivers with class=0x%x, sub_class=0x%x, protocol=0x%x",
               in_class, in_sub_class, in_protocol );

    /* -- Lock here -- */
    spin_lock_irqsave(&g_core->lock, flags);

    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        DEBUG_INFO("+ \"%s\" : config = 0x%x, interface = 0x%x",
                   tmp_func_drv->func_drv->function,
                   tmp_func_drv->func_drv->config,
                   tmp_func_drv->func_drv->interface);

        // alt_numは現在のalt_numを使用する
        if_table = get_if_table_desc( g_core, tmp_func_drv->func_drv->config, tmp_func_drv->func_drv->interface, g_core->set_interface_info.alt_num );
        DEBUG_INFO(" =>ifTbl =%p: class = 0x%x, sub_class = 0x%x, protocol = 0x%x, alt_num = 0x%x",
                   if_table,
                   if_table->uc_class,
                   if_table->uc_sub_class,
                   if_table->uc_interface_protocol,
                   g_core->set_interface_info.alt_num );

        // 3つの要素を比較する
        if( if_table && ( tmp_func_drv->started == 0 )
            && ( if_table->uc_class              == in_class )
            && ( if_table->uc_sub_class          == in_sub_class )
            && ( if_table->uc_interface_protocol == in_protocol ) ){
            DEBUG_PRINT("found!! STARTING \"%s\"", tmp_func_drv->func_drv->function);
            /* ここで条件の一致するFuncDrvへstart()通知を送る */
            PVERBOSE(" start() to %s\n",
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");
            /* ep_list情報を登録 */
            tmp_func_drv->ep_list.ep = ep_list.ep;
            tmp_func_drv->ep_list.num_ep = if_table->uc_num_pep_list;;
            DEBUG_INFO("tmp_func_drv->ep_list.num_ep=%d", tmp_func_drv->ep_list.num_ep);

            /* FuncDrvへstart()通知を送る */
            tmp_func_drv->started = 1;
            tmp_func_drv->func_drv->start_ext_info = ext_info;
            DEBUG_PRINT("start: %s", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
            tmp_func_drv->func_drv->start(tmp_func_drv->func_drv, g_core->set_interface_info.alt_num, tmp_func_drv->ep_list);
            DEBUG_PRINT("started: %s", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
        
        	/* USB給電対応で各Function毎2つのDriverが登録されているが、startさせるのは1つでよい */
        	break;
        	
        }
        else{
            DEBUG_PRINT(" || skip : %s", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
            PVERBOSE(" skip: %s\n", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
        }

    }

    /* -- Unlock Here -- */
    spin_unlock_irqrestore(&g_core->lock, flags);

    return 0;
}

/*=============================================================================
 * Export symbols
 *===========================================================================*/
EXPORT_SYMBOL(usb_gadgetcore_register_driver);
EXPORT_SYMBOL(usb_gadgetcore_unregister_driver);
EXPORT_SYMBOL(usb_gadgetcore_stop_other_driver);

module_exit(usb_gcore_module_exit);

