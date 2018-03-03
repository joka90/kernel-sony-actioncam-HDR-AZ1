/*
 * usb_gcore_ep.c
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

#include "usb_gadgetcore_cfg.h"
#include "usb_gadgetcore_pvt.h"

#ifdef DBG_PREFIX
# undef  DBG_PREFIX
# define DBG_PREFIX "GCORE_EP"
#else
# define DBG_PREFIX "GCORE_EP"
#endif
#include "usb_gcore_wrapper_cfg.h"
#include "usb_gcore_wrapper_pvt.h"

static struct usb_ep *get_ep(struct usb_gadget*);
static struct usb_ep *find_ep(struct g_core_drv*, u8);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static struct usb_request *gcore_alloc_request(struct usb_ep*, gfp_t);
static int gcore_queue (struct usb_ep*, struct usb_request*, gfp_t);
#else
static struct usb_request *gcore_alloc_request(struct usb_ep*, int);
static int gcore_queue (struct usb_ep*, struct usb_request*, int);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static void *gcore_alloc_buffer(struct usb_ep*, unsigned, dma_addr_t*, gfp_t);
#else
static void *gcore_alloc_buffer(struct usb_ep*, unsigned, dma_addr_t*, int);
#endif
static void gcore_free_buffer(struct usb_ep*, void*, dma_addr_t, unsigned);

#endif

static void gcore_free_request(struct usb_ep*, struct usb_request*);
static void gcore_complete(struct usb_ep*, struct usb_request*);
static int gcore_dequeue (struct usb_ep*, struct usb_request*);
static void gcore_fifo_flush(struct usb_ep*);
static int gcore_fifo_status(struct usb_ep*);
static int gcore_set_halt(struct usb_ep*, int);

static struct usb_ep_ops gep_ops = {
    .enable         = NULL,
    .disable        = NULL,
    
    .alloc_request  = gcore_alloc_request,
    .free_request   = gcore_free_request,
    
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
    .alloc_buffer   = gcore_alloc_buffer,
    .free_buffer    = gcore_free_buffer,
#endif
    
    .queue          = gcore_queue,
    .dequeue        = gcore_dequeue,
    
    .set_halt       = gcore_set_halt,
    .fifo_status    = gcore_fifo_status,
    .fifo_flush     = gcore_fifo_flush,
};

// Added for DualMode.
static struct usb_ep_ops gep_ops_gcw = {
    .enable         = NULL,
    .disable        = NULL,

    .alloc_request  = gcw_alloc_request,
    .free_request   = gcw_free_request,

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
    .alloc_buffer   = gcw_alloc_buffer,
    .free_buffer    = gcw_free_buffer,
#endif

    .queue          = gcw_queue,
    .dequeue        = gcw_dequeue,

    .set_halt       = gcw_set_halt,
    .fifo_status    = gcw_fifo_status,
    .fifo_flush     = gcw_fifo_flush,
};


/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * ep_ctrl_enable_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    gepを有効にする
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          in_num     : Interface番号
 *          element    : EP Descriptor
 * 出力:    !0   : 成功(ControllerDriverから貰ったepへのポインタ)
 *          NULL : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
struct usb_ep
*ep_ctrl_enable_ep(struct g_core_drv *g_core, u8 in_num, 
                   usb_gadget_if_table_element *element)
{
    struct usb_ep *ep;
    struct g_ep_info *ep_info;
    struct usb_endpoint_descriptor _ep_desc;
    
    DEBUG_INFO("%s", __FUNCTION__);
    /* Pipe List 表示 */
    PVERBOSE("========================================================\n");
    PVERBOSE("uc_ep_address: 0x%02x\n", element->uc_ep_address);
    PVERBOSE("uc_attributes: 0x%02x\n", element->uc_attributes);
    PVERBOSE("us_max_psize : %4u\n", element->us_max_psize);
    PVERBOSE("uc_interval  : %4u\n", element->uc_interval);
    PVERBOSE("uc_buffer_multiplex: %4u\n", element->uc_buffer_multiplex);
    PVERBOSE("uc_ep_restrictions:  0x%02x\n", element->uc_ep_restrictions);
    PVERBOSE("========================================================\n");
    
    /* EP Desc を作る */
    _ep_desc.bLength = USB_DT_ENDPOINT_SIZE;
    _ep_desc.bDescriptorType = USB_DT_ENDPOINT;
    _ep_desc.bEndpointAddress = element->uc_ep_address;
    _ep_desc.bmAttributes = element->uc_attributes;
    _ep_desc.wMaxPacketSize = cpu_to_le16(element->us_max_psize);
    _ep_desc.bInterval = element->uc_interval;
    
    /* 未使用のepを取得する */
    ep = get_ep(g_core->gadget);
    if(!ep){
        PDEBUG("Free EP Not found\n");
        return NULL;
    }
    PDEBUG("cep: 0x%08lx ep_info: 0x%08lx\n",
           (unsigned long)ep, (unsigned long)ep->driver_data);
    
    /* epを有効にする */
    PDEBUG("EP Enable\n");
    if(usb_ep_enable(ep, &_ep_desc) != 0){
        PDEBUG(" --->fail\n");
        return NULL;
    }
    
    /* epからep_infoを求める */
    ep_info = ep->driver_data;
    if(!ep_info){
        PDEBUG("Error: ep_info is NULL\n");
        return NULL;
    }
    
    ep_info->use = 1;
    ep_info->ep_halt = 0;
    ep_info->interface_num = in_num;
    ep_info->ep_adr = element->uc_ep_address;
    
    return ep;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * ep_ctrl_disable_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    epを無効にする
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          in_num     : Interface番号
 * 出力:    !0   : 成功
 *          0    : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
int ep_ctrl_disable_ep(struct g_core_drv *g_core, u8 in_num)
{
    struct usb_ep *ep;
    struct g_ep_info *ep_info;
    int rc;
    
    DEBUG_INFO("%s", __FUNCTION__);
    rc = 0;
    list_for_each_entry(ep, &g_core->gadget->ep_list, ep_list){
        ep_info = ep->driver_data;
        if(!ep_info) continue;
        
        /* epが使用中 かつ Interface番号が指定の番号と一致したらdisable */
        if(ep_info->use                     &&
           ep_info->interface_num == in_num    ){
            
            PDEBUG("usb_ep_disable() ep_adr: %x\n", ep_info->ep_adr);
            rc = usb_ep_disable(ep);
            if(rc == 0){
                ep_info->use = 0;
            }else{
                PDEBUG(" --->Fail rc: %d\n", rc);
                rc = -1;
            }
        }
    }
    
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * ep_ctrl_create_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    ControllerDriverから貰ったepから、FuncDrvへ渡すepを生成する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          ep         : usb_epへのポインタ(ControllerDriverから貰ったep)
 * 出力:    !0   : 成功(ep(FuncDrvに渡す用)へのポインタ)
 *          NULL : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
struct usb_ep
*ep_ctrl_create_ep(struct g_core_drv *g_core, struct usb_ep *ep)
{
    struct g_core_ep *gep;
    
    DEBUG_INFO("%s", __FUNCTION__);
    /* FuncDrv別のep用の領域を生成 */
    gep = kmalloc(sizeof(*gep), GFP_ATOMIC);
    DEBUG_NEW(gep);
    if(!gep){
        PDEBUG("Error: kmalloc()\n");
        return NULL;
    }
    
    gep->ep.driver_data = NULL;
    gep->ep.name = ep->name;
    gep->ep.ops = &gep_ops_gcw; //Changed for DualMode.
    gep->ep.ep_list = ep->ep_list;
    gep->ep.maxpacket = ep->maxpacket;
    gep->_ep = ep;
    
    //Added for DualMode.
    usb_gcw_register_ep_ops(&gep_ops);
    
    PVERBOSE("ep_ctrl_create_ep() EP: 0x%02x ------------------------\n",
           ((struct g_ep_info*)gep->_ep->driver_data)->ep_adr);
    PVERBOSE("gep:                   0x%lx\n", (unsigned long)gep);
    PVERBOSE("gep->_ep:              0x%lx\n", (unsigned long)gep->_ep);
    PVERBOSE("&gep->ep:              0x%lx\n", (unsigned long)&gep->ep);
    PVERBOSE("gep->_ep->driver_data: 0x%lx\n", (unsigned long)gep->_ep->driver_data);
    PVERBOSE("gep->_ep->ops:         0x%lx\n", (unsigned long)gep->_ep->ops);
    PVERBOSE("gep->ep.ops:           0x%lx\n\n", (unsigned long)gep->ep.ops);
    
    return &gep->ep;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * ep_ctrl_delete_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    epを削除する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          ep         : 消去するep (FuncDrvに渡す用) へのポインタ
 * 出力:    0   : 成功
 *          !0  : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
int
ep_ctrl_delete_ep(struct g_core_drv *g_core, struct usb_ep *ep)
{
    struct g_core_ep *gep;
    
    DEBUG_INFO("%s", __FUNCTION__);
    gep = container_of(ep, struct g_core_ep, ep);
    
    PVERBOSE("kfree() gep: 0x%lx\n", (unsigned long)gep);
    DEBUG_FREE( gep );
    kfree(gep);
    
    //Added for DualMode.
    usb_gcw_register_ep_ops(&gep_ops);

    return 0;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * ep_ctrl_get_in_num
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    ep_adrからInterface番号を取得する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          ep_adr     : EPアドレス
 * 出力:    !USB_GCORE_INT_NOT_DEF : Interface番号
 *          USB_GCORE_INT_NOT_DEF  : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
u8
ep_ctrl_get_in_num(struct g_core_drv *g_core, u8 ep_adr)
{
    u8 rc;
    struct usb_ep    *ep;
    struct g_ep_info *ep_info;
    
    DEBUG_INFO("%s", __FUNCTION__);
    rc = USB_GCORE_INT_NOT_DEF;
    
    list_for_each_entry(ep, &g_core->gadget->ep_list, ep_list){
        ep_info = ep->driver_data;
        if(!ep_info){
            continue;
        }
        
        if(ep_info->use              &&
           ep_info->ep_adr == ep_adr    ){
            rc = ep_info->interface_num;
            goto exit;
        }
    }
    
exit:
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * get_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    未使用のepを探す
 * 入力:    g_core     : GadgetCoreへのポインタ
 * 出力:    !0   : 成功(ControllerDriverから貰ったepへのポインタ)
 *          NULL : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static struct usb_ep
*get_ep(struct usb_gadget *gadget)
{
    struct usb_ep *ep;
    struct g_ep_info *ep_info;
    
    DEBUG_INFO("%s", __FUNCTION__);
    list_for_each_entry(ep, &gadget->ep_list, ep_list){
        ep_info = ep->driver_data;
        if(!ep_info) continue;
        if(ep_info->use == 0){
            return ep;
        }
    }
    
    return NULL;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * find_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    引数のEPアドレスに一致するepを取得する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          ep_adr     : EP Address
 * 出力:    !0   : 成功(ep(ControllerDriverから貰った)へのポインタ)
 *          NULL : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static struct usb_ep
*find_ep(struct g_core_drv *g_core, u8 ep_adr)
{
    struct usb_ep    *ep;
    struct g_ep_info *ep_info;
    
    DEBUG_INFO("%s", __FUNCTION__);
    list_for_each_entry(ep, &g_core->gadget->ep_list, ep_list){
        ep_info = ep->driver_data;
        if(!ep_info){
            continue;
        }
        
        if(ep_info->use              &&
           ep_info->ep_adr == ep_adr    ){
            return ep;
        }
    }
    
    return NULL;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * ep_ctrl_get_ep0_halt
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    ep0のHalt状態を取得する
 * 入力:    g_core     : GadgetCoreへのポインタ
 * 出力:    0    : ep0は非Stall状態
 *          1    : ep0はStall状態
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
int
ep_ctrl_get_ep0_halt(struct g_core_drv *g_core)
{
    DEBUG_INFO("%s", __FUNCTION__);
    return (g_core->ep0_halt ? 1 : 0);
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * ep_ctrl_get_ep_stall
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    引数のEPアドレスに一致するepのStall状態を取得する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          ep_adr     : EP Address
 * 出力:    0    : 該当するepは非Stall状態
 *          1    : 該当するepはStall状態
 *          -1   : 該当するepは見つからない
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
int
ep_ctrl_get_ep_stall(struct g_core_drv *g_core, u8 ep_adr)
{
    struct usb_ep *ep;
    struct g_ep_info *ep_info;
    
    DEBUG_INFO("%s", __FUNCTION__);
    /* EPアドレスからepを探す */
    ep = find_ep(g_core, ep_adr);
    if(ep == NULL){
        return -1;
    }
    
    /* epのdriver_dataからep_infoを取得 */
    ep_info = ep->driver_data;
    if(!ep_info){
        PDEBUG("Error: ep_info is null\n");
        return -1;
    }
    
    return (ep_info->ep_halt ? 1 : 0);
}


/*-------------------------------------------------------------------------*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static struct usb_request *
gcore_alloc_request (struct usb_ep *ep, gfp_t gfp_flags)
#else
static struct usb_request *
gcore_alloc_request (struct usb_ep *ep, int gfp_flags)
#endif
{
    struct g_core_ep *gep;
    struct g_core_request *greq;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    
    gep = container_of(ep, struct g_core_ep, ep);
    greq = (struct g_core_request*)kmalloc(sizeof(*greq), gfp_flags);
    DEBUG_NEW(greq);
    if(!greq){
        /* 失敗 */
        return NULL;
    }
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    PDEBUG("greq : 0x%lx\n", (unsigned long)greq);
    PVERBOSE("gep->_ep : 0x%lx\n", (unsigned long)gep->_ep);
    
    greq->_req = usb_ep_alloc_request(gep->_ep, gfp_flags);
    PVERBOSE("greq->_req : 0x%lx\n", (unsigned long)greq->_req);
    if(!greq->_req){
        /* 失敗 */
        PVERBOSE("kfree() greq: 0x%lx\n", (unsigned long)greq);
        DEBUG_FREE(greq);
        kfree(greq);
        return NULL;
    }
    
    memcpy(&greq->req, greq->_req, sizeof(greq->req));
    
    return &greq->req;
}

static void
gcore_free_request (struct usb_ep *ep, struct usb_request *req)
{
    struct g_core_ep *gep;
    struct g_core_request *greq;
    
    PDEBUG("%s call\n", __func__);
    
    gep = container_of(ep, struct g_core_ep, ep);
    greq = container_of(req, struct g_core_request, req);
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    PDEBUG("greq : 0x%lx\n", (unsigned long)greq);
    PVERBOSE("gep->_ep : 0x%lx\n", (unsigned long)gep->_ep);
    PVERBOSE("greq->_req : 0x%lx\n", (unsigned long)greq->_req);

    usb_ep_free_request(gep->_ep, greq->_req);
    PVERBOSE("kfree() greq: 0x%lx\n", (unsigned long)greq);
    DEBUG_FREE(greq);
    kfree(greq);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static void *
gcore_alloc_buffer (
        struct usb_ep           *ep,
        unsigned                bytes,
        dma_addr_t              *dma,
        gfp_t                   gfp_flags
)
#else
static void *
gcore_alloc_buffer (
        struct usb_ep           *ep,
        unsigned                bytes,
        dma_addr_t              *dma,
        int                     gfp_flags
)
#endif
{
    struct g_core_ep *gep;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    gep = container_of(ep, struct g_core_ep, ep);
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    
    return usb_ep_alloc_buffer(gep->_ep, bytes, dma, gfp_flags);
}

static void
gcore_free_buffer (
        struct usb_ep *ep,
        void *buf,
        dma_addr_t dma,
        unsigned bytes
)
{
    struct g_core_ep *gep;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    gep = container_of(ep, struct g_core_ep, ep);
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    
    usb_ep_free_buffer(gep->_ep, buf, dma, bytes);
}

#endif

static void
gcore_complete(struct usb_ep *_ep, struct usb_request *_req)
{
    struct g_core_ep *gep;
    struct g_core_request *greq;
    void   (*complete)(struct usb_ep *ep, struct usb_request *req);
    void   *context;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    
    greq = (struct g_core_request*)_req->context;
    gep = greq->gep;
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    PDEBUG("greq : 0x%lx\n", (unsigned long)greq);
    PVERBOSE("gep->_ep : 0x%lx\n", (unsigned long)gep->_ep);
    PVERBOSE("greq->_req : 0x%lx\n", (unsigned long)greq->_req);
    
    if(greq->req.complete){
        complete = greq->req.complete;
        context = greq->req.context;
        memcpy(&greq->req, _req, sizeof(greq->req));
        greq->req.complete = complete;
        greq->req.context = context;
        
        complete(&gep->ep, &greq->req);
    }
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static int
gcore_queue (struct usb_ep *ep, struct usb_request *req, gfp_t gfp_flags)
#else
static int
gcore_queue (struct usb_ep *ep, struct usb_request *req, int gfp_flags)
#endif
{
    struct g_core_ep *gep;
    struct g_core_request *greq;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    
    gep = container_of(ep, struct g_core_ep, ep);
    greq = container_of(req, struct g_core_request, req);
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    PDEBUG("greq : 0x%lx\n", (unsigned long)greq);
    PVERBOSE("gep->_ep : 0x%lx\n", (unsigned long)gep->_ep);
    PVERBOSE("greq->_req : 0x%lx\n", (unsigned long)greq->_req);
    
    memcpy(greq->_req, req, sizeof(*greq->_req));
    greq->_req->complete = gcore_complete;
    greq->_req->context = greq;
    
    greq->gep = gep;
    
    return usb_ep_queue(gep->_ep, greq->_req, gfp_flags);
}

static int
gcore_dequeue (struct usb_ep *ep, struct usb_request *req)
{
    struct g_core_ep *gep;
    struct g_core_request *greq;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    
    gep = container_of(ep, struct g_core_ep, ep);
    greq = container_of(req, struct g_core_request, req);
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    PDEBUG("greq : 0x%lx\n", (unsigned long)greq);
    PVERBOSE("gep->_ep : 0x%lx\n", (unsigned long)gep->_ep);
    PVERBOSE("greq->_req : 0x%lx\n", (unsigned long)greq->_req);
    
    return usb_ep_dequeue(gep->_ep, greq->_req);
}

static void
gcore_fifo_flush(struct usb_ep *ep)
{
    struct g_core_ep *gep;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    gep = container_of(ep, struct g_core_ep, ep);
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    
    usb_ep_fifo_flush(gep->_ep);
}

static int
gcore_fifo_status(struct usb_ep *ep)
{
    struct g_core_ep *gep;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    gep = container_of(ep, struct g_core_ep, ep);
    
    PDEBUG("gep : 0x%lx\n", (unsigned long)gep);
    
    return usb_ep_fifo_status(gep->_ep);
}

static int
gcore_set_halt(struct usb_ep *ep, int value)
{
    int rc;
    struct g_ep_info *ep_info;
    struct g_core_ep *gep;
    u8 old_ep_halt;
    
    DEBUG_INFO("%s", __FUNCTION__);
    PDEBUG("%s call\n", __func__);
    gep = container_of(ep, struct g_core_ep, ep);
    ep_info = gep->_ep->driver_data;
    
    PDEBUG("gep : 0x%lx ep_info : 0x%lx\n", 
           (unsigned long)gep, (unsigned long)ep_info);
    
    // Escape old ep_halt state.
    old_ep_halt = ep_info->ep_halt;
    
    if(value){
        PDEBUG(" -->EP Halt\n");
        ep_info->ep_halt = 1;
        rc = usb_ep_set_halt(gep->_ep);
        if(rc != 0){
            ep_info->ep_halt = old_ep_halt;
        }
    }else{
        PDEBUG(" -->EP Clear Halt\n");
        ep_info->ep_halt = 0;
        rc = usb_ep_clear_halt(gep->_ep);
        if(rc != 0){
            ep_info->ep_halt = old_ep_halt;
        }
    }
    
    return rc;
}

