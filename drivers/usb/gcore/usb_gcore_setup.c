/*
 * usb_gcore_setup.c
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
#include <linux/usb/gcore/usb_otgcore.h>

#include "usb_gadgetcore_cfg.h"
#include "usb_gadgetcore_pvt.h"

#ifdef DBG_PREFIX
# undef  DBG_PREFIX
# define DBG_PREFIX "GCORE_SETUP"
#else
# define DBG_PREFIX "GCORE_SETUP"
#endif
#include "usb_gcore_wrapper_cfg.h"
#include "usb_gcore_wrapper_pvt.h"

static int start_config(struct g_core_drv*, u8 config_num);
static int stop_config(struct g_core_drv*);
static int start_interface(struct g_core_drv*, u8, u8);
static int stop_interface(struct g_core_drv*, u8);
static int set_fifo_map(struct g_core_drv*, usb_gadget_config_desc*);

static void ep0_complete(struct usb_ep*, struct usb_request*);

static void setup_set_config_comp(struct g_core_drv*, struct usb_request*);
static void setup_set_interface_comp(struct g_core_drv*, struct usb_request*);
static int setup_class_specific(struct g_core_drv*, const struct usb_ctrlrequest*);
static int setup_vendor_specific(struct g_core_drv*, const struct usb_ctrlrequest*);
static int setup_set_config(struct g_core_drv*, const struct usb_ctrlrequest*);
static int setup_get_config(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
static int setup_set_interface(struct g_core_drv*, const struct usb_ctrlrequest*);
static int setup_get_interface(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
static int setup_set_feature(struct g_core_drv*, const struct usb_ctrlrequest*);
static int set_halt_ep0(struct g_core_drv*);
static int clear_halt_ep0(struct g_core_drv*);
static int set_halt_ep(struct g_core_drv*, u8);
static int clear_halt_ep(struct g_core_drv*, u8);
static int get_status_ep0(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
static int get_status_ep(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
static int setup_clear_feature(struct g_core_drv*, const struct usb_ctrlrequest*);
static int setup_get_status(struct g_core_drv*, const struct usb_ctrlrequest*, void*);
static int setup_get_ms_descriptor(struct g_core_drv*, const struct usb_ctrlrequest*, void*);

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * ep0_complete
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    GadgetCoreが自動で処理したEP0通信の完了Callback先
 *          usb_ep_queue(ep0, ep0req) 呼び出し時の ep0req->completeに設定
 * 入力:    *ep        : 通信完了したEPへのポインタ
 *          *req       : 通信完了したrequestへのポインタ
 * 出力:    
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static void
ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct g_core_drv *g_core;
    
    PDEBUG("ep0_complete()\n");
    
    /* EP0のdriver_dataから g_coreへのポインタを取得 */
    g_core = ep->driver_data;
    if(!g_core) return;
    
    /* EP0での通信が完了した後に呼ぶ関数が登録されていれば呼ぶ */
    if(g_core->ep0_comp_func){
        PDEBUG("    ep0_comp_func()\n");
        g_core->ep0_comp_func(g_core, req);
        
        /* EP0での通信が完了した後に呼ぶ関数を破棄する */
        g_core->ep0_comp_func = NULL;
    }
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_class_specific
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    ClassSpecificRequestをFuncDrvへ通知する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          ctrl       : SETUPパケット
 * 出力:     0 : 成功
 *           0 : STALL
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_class_specific(struct g_core_drv *g_core,
                                const struct usb_ctrlrequest *ctrl)
{
    struct g_func_drv *tmp_func_drv;
    u8 cfg_num, in_num;
    int rc;
    
    if((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE){
        /* 該当のInterfaceを求める */
        in_num = (u8)(ctrl->wIndex & 0x00ff);
        cfg_num = g_core->set_config;
        PDEBUG("ClassSpecificRequest(INTERFACE) In: %d\n", in_num);

        //---
        // DualModeによりMTP動作に状態が遷移している場合は、
        // gcwによって動作しているfuncDrvがSICに切り替わっている。
        // よって、ここではSIC用のconfigを探し、cfg_numを上書きする
        if( (get_gcw_state() == GCW_STATE_DUAL_CONVERTING) ||
            (get_gcw_state() == GCW_STATE_DUAL_FIXED_SIC) ){
            DEBUG_PRINT("gcw State is DUAL_FIXED_SIC/DUAL_CONVERTING!");
            DEBUG_PRINT("Force To Stop SIC FuncDrv in %s!!", __FUNCTION__);

            tmp_func_drv = findFuncDrv_fromIfClasses( SIC_IF_CLASS, SIC_IF_SUBCLASS, SIC_IF_PROTOCOL);

            if( (tmp_func_drv != NULL) &&
                (tmp_func_drv->func_drv != NULL) ){
                cfg_num = tmp_func_drv->func_drv->config;
            }
            DEBUG_INFO("SICs cfg_num is %d", cfg_num);
        }
        else{
            DEBUG_PRINT("gcw State is NOT DUAL_FIXED_SIC/DUAL_CONVERTING.");
            DEBUG_PRINT("So, will not stop SIC here in %s", __FUNCTION__);
        }
        //---

        /* 該当するFuncDrvを探してclass()を呼ぶ */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->config    == cfg_num &&
               tmp_func_drv->func_drv->interface == in_num     ){
                /* ここで条件の一致するFuncDrvへstart()通知を送る */
                if(tmp_func_drv->func_drv->class){
                    PVERBOSE(" class() to %s\n", 
                             tmp_func_drv->func_drv->function ?
                             tmp_func_drv->func_drv->function : "");
                    rc = tmp_func_drv->func_drv->class(tmp_func_drv->func_drv,
                                                          ctrl, g_core->ep0);
                    if(rc == 0){
                        /* 0を返すFuncDrvがあればそこで終了 */
                        /* EP0のHalt状態を解除する */
                        clear_halt_ep0(g_core);
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALLを返すFuncDrvがあればそこで終了 */
                        PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                        rc = -EINVAL;
                        goto exit;
                    }
                    PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
                }
            }
        }
        /* どのFuncDrvも処理をしない場合はStallにする */
        rc = -EINVAL;
    }else if((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT){
        /* 指定されたep_adrの属するCfg番号を求める */
        cfg_num = g_core->set_config;

        //---
        // DualModeによりMTP動作に状態が遷移している場合は、
        // gcwによって動作しているfuncDrvがSICに切り替わっている。
        // よって、ここではSIC用のconfigを探し、cfg_numを上書きする
        if( (get_gcw_state() == GCW_STATE_DUAL_CONVERTING) ||
            (get_gcw_state() == GCW_STATE_DUAL_FIXED_SIC) ){
            DEBUG_PRINT("gcw State is DUAL_FIXED_SIC/DUAL_CONVERTING!");
            DEBUG_PRINT("Force To Stop SIC FuncDrv in %s!!", __FUNCTION__);

            tmp_func_drv = findFuncDrv_fromIfClasses( SIC_IF_CLASS, SIC_IF_SUBCLASS, SIC_IF_PROTOCOL);

            if( (tmp_func_drv != NULL) &&
                (tmp_func_drv->func_drv != NULL) ){
                cfg_num = tmp_func_drv->func_drv->config;
            }
            DEBUG_INFO("SICs cfg_num is %d", cfg_num);
        }
        else{
            DEBUG_PRINT("gcw State is NOT DUAL_FIXED_SIC/DUAL_CONVERTING.");
            DEBUG_PRINT("So, will not stop SIC here in %s", __FUNCTION__);
        }
        //---

        /* 指定されたep_adrの属するIn番号を求める */
        in_num = ep_ctrl_get_in_num(g_core, (ctrl->wIndex & 0x00ff));
        if(in_num == USB_GCORE_INT_NOT_DEF){
            PDEBUG("ep_adr not found\n");
            rc = -EINVAL;
            goto exit;
        }

        /* 該当するFuncDrvを探してclass()を呼ぶ */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->config    == cfg_num &&
               tmp_func_drv->func_drv->interface == in_num     ){
                /* ここで条件の一致するFuncDrvへstart()通知を送る */
                if(tmp_func_drv->func_drv->class){
                    PVERBOSE(" class() to %s\n", 
                             tmp_func_drv->func_drv->function ?
                             tmp_func_drv->func_drv->function : "");
                    rc = tmp_func_drv->func_drv->class(tmp_func_drv->func_drv,
                                                          ctrl, g_core->ep0);
                    if(rc == 0){
                        /* 0を返すFuncDrvがあればそこで終了 */
                        /* EP0のHalt状態を解除する */
                        clear_halt_ep0(g_core);
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALLを返すFuncDrvがあればそこで終了 */
                        PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                        rc = -EINVAL;
                        goto exit;
                    }
                    PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
                }
            }
        }
        /* どのFuncDrvも処理をしない場合はStallにする */
        rc = -EINVAL;
    }else{   /* Device,Other宛 */
        /* 該当するFuncDrvを探してclass()を呼ぶ */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->class){
                PVERBOSE(" class() to %s\n", 
                         tmp_func_drv->func_drv->function ?
                         tmp_func_drv->func_drv->function : "");
                rc = tmp_func_drv->func_drv->class(tmp_func_drv->func_drv,
                                                      ctrl, g_core->ep0);
                if(rc == 0){
                    /* 0を返すFuncDrvがあればそこで終了 */
                    /* EP0のHalt状態を解除する */
                    clear_halt_ep0(g_core);
                    goto exit;
                }else if(rc == USB_GADGETCORE_STALL){
                    /* STALLを返すFuncDrvがあればそこで終了 */
                    PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                    rc = -EINVAL;
                    goto exit;
                }
                PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
            }
        }
        /* どのFuncDrvも処理をしない場合はStallにする */
        rc = -EINVAL;
    }
    
exit:
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_vendor_specific
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    ClassSpecificRequestをFuncDrvへ分配する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          ctrl       : SETUPパケット
 * 出力:     0 : 成功
 *           0 : STALL
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_vendor_specific(struct g_core_drv *g_core,
                                 const struct usb_ctrlrequest *ctrl)
{
    struct g_func_drv *tmp_func_drv;
    u8 cfg_num, in_num;
    int rc;
    
    if((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE){
        /* 該当のInterfaceを求める */
        in_num = (u8)(ctrl->wIndex & 0x00ff);
        cfg_num = g_core->set_config;
        PDEBUG("VendorSpecificRequest(INTERFACE) In: %d\n", in_num);

        /* 該当するFuncDrvを探してvendor()を呼ぶ */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->config    == cfg_num &&
               tmp_func_drv->func_drv->interface == in_num     ){
                /* ここで条件の一致するFuncDrvへstart()通知を送る */
                if(tmp_func_drv->func_drv->vendor){
                    PVERBOSE(" vendor() to %s\n", 
                             tmp_func_drv->func_drv->function ?
                             tmp_func_drv->func_drv->function : "");
                    rc = tmp_func_drv->func_drv->vendor(tmp_func_drv->func_drv,
                                                           ctrl, g_core->ep0);
                    if(rc == 0){
                        /* 0を返すFuncDrvがあればそこで終了 */
                        /* EP0のHalt状態を解除する */
                        clear_halt_ep0(g_core);
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALLを返すFuncDrvがあればそこで終了 */
                        PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                        rc = -EINVAL;
                        goto exit;
                    }
                    PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
                }
            }
        }
        /* どのFuncDrvも処理をしない場合はStallにする */
        rc = -EINVAL;
    }else if((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT){
        /* 指定されたep_adrの属するCfg,In番号を求める */
        cfg_num = g_core->set_config;
        
        in_num = ep_ctrl_get_in_num(g_core, (ctrl->wIndex & 0x00ff));
        if(in_num == USB_GCORE_INT_NOT_DEF){
            PDEBUG("ep_adr not found\n");
            rc = -EINVAL;
            goto exit;
        }
        
        /* 該当するFuncDrvを探してvendor()を呼ぶ */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->config    == cfg_num &&
               tmp_func_drv->func_drv->interface == in_num     ){
                /* ここで条件の一致するFuncDrvへstart()通知を送る */
                if(tmp_func_drv->func_drv->vendor){
                    PVERBOSE(" vendor() to %s\n", 
                             tmp_func_drv->func_drv->function ?
                             tmp_func_drv->func_drv->function : "");
                    rc = tmp_func_drv->func_drv->vendor(tmp_func_drv->func_drv,
                                                           ctrl, g_core->ep0);
                    if(rc == 0){
                        /* 0を返すFuncDrvがあればそこで終了 */
                        /* EP0のHalt状態を解除する */
                        clear_halt_ep0(g_core);
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALLを返すFuncDrvがあればそこで終了 */
                        PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                        rc = -EINVAL;
                        goto exit;
                    }
                    PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
                }
            }
        }
        /* どのFuncDrvも処理をしない場合はStallにする */
        rc = -EINVAL;
    }else{   /* Device,Other宛 */

        /* Check for GET_MS_DESCRIPTOR in case when the request is to Device */
        if(ctrl->bRequestType == (USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE) ){

            /* Check if the request is a valid GET_MS_DESCRIPTOR */
            if( (ctrl->bRequest == USB_FEATURE_DESCRIPTOR_CODE ) &&
                (ctrl->wValue   == ( (USB_REQ_FEATURE_DESC_WVALUE_H*0x0100) | (USB_REQ_FEATURE_DESC_WVALUE_L) ) ) &&
                (ctrl->wIndex   == USB_REQ_FEATURE_DESC_WINDEX ) ){

                /* in case NO DUAL MODE, it should not return the response*/
                if( g_core->enable_dual_mode == 0 ){
                    PVERBOSE("  --->rc: USB_GADGETCORE_STALL, not in DualMode.\n");
                    DEBUG_PRINTK("\x1b[7;31mRecved 0x%x! but not DUAL_MODE\x1b[0m", ctrl->bRequest);
                    rc = -EINVAL;
                    goto exit;
                }

                /* Create the response for GET_MS_DESCRITPOR */
                PVERBOSE("GET_MS_DESCRIPTOR[0x%x]", ctrl->wValue >> 8);
                DEBUG_PRINTK("\x1b[7;35mRecved 0x%x! GET_MS_DESCRIPTOR\x1b[0m", ctrl->bRequest);
                rc = setup_get_ms_descriptor(g_core, ctrl, g_core->ep0req->buf);

                /* Dump to display for debug*/
                DEBUG_DUMP(g_core->ep0req->buf, rc);

                /* Get ready for ep0_queue */
                g_core->ep0req->length = rc;
                g_core->ep0req->complete = ep0_complete;
                g_core->ep0req->zero = (rc < ctrl->wLength &&
                                        (rc % g_core->gadget->ep0->maxpacket) == 0);
                g_core->ep0->driver_data = g_core;

                PVERBOSE("usb_ep_queue(EP0)\n");
                PVERBOSE(" buf:    %lx\n", (unsigned long)g_core->ep0req->buf);
                PVERBOSE(" length: %d\n", g_core->ep0req->length);
                PVERBOSE(" dma: %d\n", (unsigned int)g_core->ep0req->dma);
                PVERBOSE(" no_interrupt: %d\n", g_core->ep0req->no_interrupt);
                PVERBOSE(" zero: %d\n", g_core->ep0req->zero);
                PVERBOSE(" short_not_ok: %d\n", g_core->ep0req->short_not_ok);

                /* send the response */
                rc = usb_ep_queue(g_core->ep0, g_core->ep0req, GFP_ATOMIC);

                goto exit;
            }
        }

        /* 該当するFuncDrvを探してvendor()を呼ぶ */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->vendor){
                PVERBOSE(" vendor() to %s\n", 
                         tmp_func_drv->func_drv->function ?
                         tmp_func_drv->func_drv->function : "");
                rc = tmp_func_drv->func_drv->vendor(tmp_func_drv->func_drv,
                                                       ctrl, g_core->ep0);
                if(rc == 0){
                    /* 0を返すFuncDrvがあればそこで終了 */
                    /* EP0のHalt状態を解除する */
                    clear_halt_ep0(g_core);
                    goto exit;
                }else if(rc == USB_GADGETCORE_STALL){
                    /* STALLを返すFuncDrvがあればそこで終了 */
                    PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                    rc = -EINVAL;
                    goto exit;
                }
                PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
            }
        }
        /* どのFuncDrvも処理をしない場合はStallにする */
        rc = -EINVAL;
    }
    
exit:
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * start_config
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    指定のConfigで動作するepをenableにし、
 *          Config、Interfaceが一致するFuncDrvへstart()を通知する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          config_num : Config番号
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int start_config(struct g_core_drv *g_core, u8 config_num)
{
    usb_gadget_config_desc *cfg_desc;
    int rc;
    u8 i;
    
    /* Config値が0ならば値だけ保存して終了 */
    if(config_num == 0){
        g_core->set_config = 0;
        return 0;
    }
    
    /* Descriptor Table から指定のConfig番号のConfigDescriptorを取得 */
    cfg_desc = get_config_desc_from_config_num(g_core, config_num);
    if(cfg_desc == NULL){
        return -1; /* 見つからない */
    }
    
    /* FIFOを設定する */
    rc = set_fifo_map(g_core, cfg_desc);
    if(rc != 0){
        return -1;
    }
    
    /* 新しいConfig値として保存 */
    g_core->set_config = config_num;
    
    /* Alternate番号保存領域を設定 */
    make_alt_num_list(g_core);
    
    /* Config内のInterfaceを生成する */
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        rc = start_interface(g_core, cfg_desc->interfaces[i].uc_if_number, 0);
        if(rc){
            PDEBUG("start_config() Error\n");
            return -1;
        }
    }
    
    return 0;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_disconnect
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    disconnectされた時の処理
 * 入力:    g_core     : GadgetCoreへのポインタ
 * 出力:    
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
void
setup_disconnect(struct g_core_drv *g_core)
{
    
    unsigned long flags;
    
    //lock
    spin_lock_irqsave(&g_core->lock_stop_config, flags);
    
    /* 今のConfigを停止 */
    stop_config(g_core);
    
    /* EP0のhalt状態を解除 */
    clear_halt_ep0(g_core);
    
    //unlock
    spin_unlock_irqrestore(&g_core->lock_stop_config, flags);
    
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * stop_config
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    引数のConfig番号で動いているFuncDrvへstop()を通知し
 *          epを全てdisableにする
 * 入力:    g_core     : GadgetCoreへのポインタ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int stop_config(struct g_core_drv *g_core)
{
    usb_gadget_config_desc *cfg_desc;
    int rc;
    u8 i;
    
    /* 現在のConfig値が0ならば何もしない */
    if(g_core->set_config == 0){
        return 0;
    }
    
    /* 現在のConfigDescriptorを取得する */
    cfg_desc = get_config_desc_from_config_num(g_core, g_core->set_config);
    if(cfg_desc == NULL){
        PDEBUG(" --->Fail\n");
        return -1; /* 見つからない */
    }
    
    /* Alternate番号保存領域を開放 */
    free_alt_num_list(g_core);
    
    /* Config内のInterfaceを終了する */
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        rc = stop_interface(g_core, cfg_desc->interfaces[i].uc_if_number);
        if(rc != 0){
            PDEBUG("stop_config() Error\n");
            return -1;
        }
    }
    
    /* Config値を0にする */
    g_core->set_config = 0;
    
    /* RemoteWakeup許可状態をクリア */
    g_core->dev_feature = 0;
    
    /* FIFOの設定をリセットする */
    PVERBOSE("usb_gadget_ioctl(USB_IOCTL_FIFO_RESET)\n");
    rc = usb_gadget_ioctl(g_core->gadget, USB_IOCTL_FIFO_RESET, 0);
    if(rc != 0){
        PDEBUG("usb_gadget_ioctl(USB_IOCTL_FIFO_RESET) Error\n");
        return -1;
    }
    
    return 0;
}


/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * set_fifo_map
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    受け取ったConfig DescriptorからFIFO領域を設定する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          cfg_desc  : 現在のSpeed、Confi番号の Config Descriptor へのポインタ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int set_fifo_map(struct g_core_drv *g_core, usb_gadget_config_desc *cfg_desc)
{
    usb_gadget_interface_desc *if_desc;
    usb_gadget_if_table *setting;
    usb_gadget_if_table_element *ift_element;
    struct usb_ioctl_fifo_param fifo_param;
    int rc;
    u8 ep_num;  /* EP番号(EPアドレスではない) */
    u8 ep_adr = 0;  /* EPアドレス (EP番号 + Direction) */
    u8 type = 0;
    u16 max_mps = 0;
    u8 int_num, exist_ep;
    u8 i, j, k;
    
    PVERBOSE("----------------------------------\n");
    PVERBOSE("set_fifo_map()\n");
    
    /* 念のためにFIFOをクリアする */
    rc = usb_gadget_ioctl(g_core->gadget, USB_IOCTL_FIFO_RESET, 0);
    if(rc != 0){
        PDEBUG("usb_gadget_ioctl(USB_IOCTL_FIFO_RESET) Error\n");
        return -1;
    }
    
    /* EP番号 1-15 を順に見ていく */
    rc = 0;
    for(ep_num = 1; ep_num <= 15; ep_num++){
        PVERBOSE("  ep_num: %d\n", ep_num);
        
        int_num = USB_GCORE_INT_NOT_DEF;
        for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
            if_desc = &cfg_desc->interfaces[i];
            
            for(j = 0; j < if_desc->uc_num_if_tables; j++){
                setting = &if_desc->if_tables[j];
                
                exist_ep = 0;
                for(k = 0; k < setting->uc_num_pep_list; k++){
                    ift_element = &setting->pep_list[k];
                    
                    if((ift_element->uc_ep_address & USB_ENDPOINT_NUMBER_MASK) == ep_num){
                        PVERBOSE("Hit!!\n");
                        
                        if(exist_ep){
                            rc = -1;
                            goto exit;
                        }
                        exist_ep = 1;
                        
                        /* 同じEP番号がどのInterfaceで定義されているか確認 */
                        if(int_num == USB_GCORE_INT_NOT_DEF){
                            /* MaxPacketSize を 保存 */
                            max_mps = ift_element->us_max_psize;
                            
                            /* EP Address を保存 */
                            ep_adr = ift_element->uc_ep_address;
                            
                            type = ift_element->uc_buffer_multiplex;
                            
                            /* Interface番号を保存 */
                            int_num = if_desc->uc_if_number;
                            
                        }else if(int_num == if_desc->uc_if_number){
                            /* EP Address を比較 */
                            if(ep_adr != ift_element->uc_ep_address){
                                rc = -1;
                                goto exit;
                            }
                            
                            if(type != ift_element->uc_buffer_multiplex){
                                rc = -1;
                                goto exit;
                            }
                            
                            /* 最大のMaxPacketSizeを求める */
                            max_mps = max(max_mps, ift_element->us_max_psize);
                        }else{
                            rc = -1;
                            goto exit;
                        }
                    }
                }
            }
        }
        
        if(int_num != USB_GCORE_INT_NOT_DEF){
            /* FIFO設定用パラメータ設定 */
            fifo_param.bEpAdr = ep_adr;
            fifo_param.wSize = max_mps;
            fifo_param.bType = type;
            
            /* IOCTL_FIFO_MAPを呼ぶ */
            PDEBUG("usb_gadget_ioctl(USB_IOCTL_FIFO_MAP)\n");
            PDEBUG(" bEpAdr: %x wSize: %d bType: %d\n", 
                         fifo_param.bEpAdr, fifo_param.wSize, fifo_param.bType);
            rc = usb_gadget_ioctl(g_core->gadget, USB_IOCTL_FIFO_MAP, (unsigned long)&fifo_param);
            if(rc != 0){
                PDEBUG("usb_gadget_ioctl(USB_IOCTL_FIFO_MAP) Error\n");
                return -1;
            }
        }
    }
    
#if CONFIG_USB_GADGET_CORE_LOGGING > 0
    usb_gadget_ioctl(g_core->gadget, USB_IOCTL_FIFO_DUMP, 0);
#endif

exit:
    if(rc != 0){
        /* 正常終了でなければFIFOをクリアする */
        PVERBOSE("usb_gadget_ioctl(USB_IOCTL_FIFO_RESET)\n");
        usb_gadget_ioctl(g_core->gadget, USB_IOCTL_FIFO_RESET, 0);
    }
    
    return rc;
}

#define USB_GADGET_CORE_UC_MAX_POWER_DEFAULT ( 0x32 )   // means 100mA.
/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * start_interface
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    受け取ったInterface番号とAlternate番号から
 *          epを生成し、Config,Interfaceの一致するFuncDrvへstart()通知
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          in_num    : Interface番号
 *          alt_num   : Alternate番号
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int start_interface(struct g_core_drv *g_core, u8 in_num, u8 alt_num)
{
    struct g_func_drv *tmp_func_drv;
    struct usb_ep **eps = NULL, **feps = NULL;
    usb_gadget_if_table *if_table;
    usb_gadget_if_table_element *element;
    u8 i;
    int rc;
    int special_start_rsv = 0;
    __u8 ucMaxPower = USB_GADGET_CORE_UC_MAX_POWER_DEFAULT;
    usb_gadget_config_desc *pCconfigDesc;
    
    PDEBUG("start_interface() Cfg: %d In: %d Alt: %d\n",
                                  g_core->set_config, in_num, alt_num);
    
    /* 開始する Config,Interface に該当するInterfaceTableを取得する */
    if_table = get_if_table_desc(g_core, g_core->set_config, in_num, alt_num);
    if(!if_table){
        PDEBUG("Error: Not found if_table\n");
        rc = -1;
        goto exit;
    }
    
    /* マスターEPリスト領域を確保する */
    if(if_table->uc_num_pep_list != 0){
        eps = kmalloc(sizeof(struct usb_ep*) * if_table->uc_num_pep_list, 
                      GFP_ATOMIC);
        DEBUG_NEW(eps);
        PVERBOSE("kmalloc() eps: 0x%lx\n", (unsigned long)eps);
        if(!eps){
            PDEBUG("Error: kmalloc()\n");
            rc = -1;
            goto exit;
        }
    }
    
    /* マスターEPリストを作成 */
    for(i = 0; i < if_table->uc_num_pep_list; i++){
        element = &if_table->pep_list[i];
        *(eps + i) = ep_ctrl_enable_ep(g_core, in_num, element);
        
        if(!*(eps + i)){
            rc = -1;
            goto exit;
        }
    }
    
    /* FuncDriverへstart()通知する */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->config == g_core->set_config &&
           tmp_func_drv->func_drv->interface == in_num             ){
            
            DEBUG_PRINT("start() to \"%s\"", 
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");

            /* FuncDrv用EPリスト領域を確保する */
            if(if_table->uc_num_pep_list != 0){
                feps = kmalloc(sizeof(struct usb_ep*) * if_table->uc_num_pep_list, 
                               GFP_ATOMIC);
                DEBUG_NEW(feps);
                PVERBOSE("feps: 0x%lx\n", (unsigned long)feps);
                if(!feps){
                    PDEBUG("Error: kmalloc()\n");
                    
                    /* エラーが発生したら、interfaceを停止して終了 */
                    stop_interface(g_core, in_num);
                    rc = -1;
                    goto exit;
                }
            }
            
            /* FuncDrv用EPリストを作成 */
            for(i = 0; i < if_table->uc_num_pep_list; i++){
                *(feps + i) = ep_ctrl_create_ep(g_core, *(eps+i));
                if(!*(feps + i)){
                    PDEBUG("Error: kmalloc()\n");
                    
                    /* エラーが発生したら、今まで作ったepを削除 */
                    while(i > 0){
                        i--;
                        ep_ctrl_delete_ep(g_core, *(feps + i));
                    }
                    
                    /* interfaceを停止して終了 */
                    stop_interface(g_core, in_num);
                    rc = -1;
                    goto exit;
                }
            }
            
            /* ep_listを作成 */
            tmp_func_drv->ep_list.ep = feps;
            tmp_func_drv->ep_list.num_ep = if_table->uc_num_pep_list;;
            
            /* FuncDrvへstart()通知を送る */
            PVERBOSE(" start() to %s\n", 
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");
            tmp_func_drv->started = 1;
            tmp_func_drv->func_drv->start_ext_info = USB_GCORE_STARTEXT_NORMAL;
            special_start_rsv = 1;

            tmp_func_drv->func_drv->start(tmp_func_drv->func_drv,
                                             alt_num, tmp_func_drv->ep_list);
            
            pCconfigDesc = get_current_config_desc( g_core );
            
            if( pCconfigDesc ){
                
                ucMaxPower = pCconfigDesc->uc_max_power;
                
            }
        }
    }
    
    // Notify start to special function driver if some driver started.
    if( special_start_rsv ){
        
        /* Special FuncDriverへstart()通知する */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if( ( tmp_func_drv->func_drv->config    == 0xFF ) &&
                ( tmp_func_drv->func_drv->interface == 0xFF ) ){
                
                tmp_func_drv->func_drv->start( 
                    tmp_func_drv->func_drv,
                    ucMaxPower,
                    tmp_func_drv->ep_list );
                
            }
        }
        
    }
    
exit:
    /* マスターEPリスト領域を開放する */
    if(eps){
        PVERBOSE("kfree(eps) eps: 0x%lx\n", (unsigned long)eps);
        DEBUG_FREE(eps);
        kfree(eps);
    }
    
    return 0;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * stop_interface
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    引数のInterface番号で動いているFuncDrvへstop()を通知し
 *          epを全てdisableにする
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          in_num     : Interface番号
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int stop_interface(struct g_core_drv *g_core, u8 in_num)
{
    struct g_func_drv *tmp_func_drv;
    struct usb_ep **eps = NULL;
    u8 cfg_num, i;
    int rc;
    int stop_rsv = 0;
    
    cfg_num = g_core->set_config;
    
    PDEBUG("stop_interface() Cfg: %d In: %d\n", cfg_num, in_num);

    // DualModeによりMTP動作に状態が遷移している場合は、
    // gcwによって動作しているfuncDrvがSICに切り替わっている。
    // よって、ここではSIC用のconfigを探し、cfg_numを上書きする
    if( (get_gcw_state() == GCW_STATE_DUAL_CONVERTING) ||
        (get_gcw_state() == GCW_STATE_DUAL_FIXED_SIC) ){
        DEBUG_PRINT("gcw State is DUAL_FIXED_SIC/DUAL_CONVERTING!");
        DEBUG_PRINT("Force To Stop SIC FuncDrv in %s!!", __FUNCTION__);

        tmp_func_drv = findFuncDrv_fromIfClasses( SIC_IF_CLASS, SIC_IF_SUBCLASS, SIC_IF_PROTOCOL);

        if( (tmp_func_drv != NULL) &&
            (tmp_func_drv->func_drv != NULL) ){
            cfg_num = tmp_func_drv->func_drv->config;
        }
        DEBUG_INFO("SICs cfg_num is %d", cfg_num);
    }
    else{
        DEBUG_PRINT("gcw State is NOT DUAL_FIXED_SIC/DUAL_CONVERTING.");
        DEBUG_PRINT("So, will not stop SIC here in %s", __FUNCTION__);
    }

    /* 現在のConfig番号、指定のInterface番号で動作しているFuncDrvに
       対してstop()を通知する */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->config == cfg_num   &&
           tmp_func_drv->func_drv->interface == in_num &&
           tmp_func_drv->started != 0           ){
            
            /* ここで条件の一致するFuncDrvへstop()通知を送る */
            PVERBOSE(" stop() to %s\n", 
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");
            tmp_func_drv->started = 0;
            stop_rsv = 1;
            DEBUG_PRINT("stop() to \"%s\"", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
            tmp_func_drv->func_drv->stop(tmp_func_drv->func_drv);
            DEBUG_PRINT("stopped \"%s\"", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");

            eps = tmp_func_drv->ep_list.ep;
            
            /* FuncDrv用EPリストを削除 */
            for(i = 0; i < tmp_func_drv->ep_list.num_ep; i++){
                ep_ctrl_delete_ep(g_core, *(eps+i));
            }
            
            if(eps != NULL){
                PVERBOSE("kfree() eps: 0x%lx\n", (unsigned long)eps);
                DEBUG_FREE(eps);
                kfree(eps);
            }
        }
    }
    
    if( stop_rsv ){
        
        list_for_each_entry( tmp_func_drv, &g_core->func_list, list ){
            
            if( ( tmp_func_drv->func_drv->config    == 0xFF ) &&
                ( tmp_func_drv->func_drv->interface == 0xFF ) ){
                
                tmp_func_drv->func_drv->stop( tmp_func_drv->func_drv );
                
            }
            
        }
        
    }
    
    /* Interface内の全てのEPをdisableにする */
    rc = ep_ctrl_disable_ep(g_core, in_num);
    if(rc){
        PDEBUG("Error: ep_ctrl_disable_ep()\n");
    }
    
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_set_config
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    スタンダードリクエスト SetConfig を処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int
setup_set_config(struct g_core_drv *g_core, const struct usb_ctrlrequest *ctrl)
{
    int rc;
    u8 old_config, new_config;
    
    /* USB2.0spec 9.4.7 : The upper byte of the wValue field is reserved. */
    new_config = (u8)(ctrl->wValue & 0x00ff);
    old_config = g_core->set_config;
    
    PDEBUG("==========================================================\n");
    PDEBUG("SetConfig Cfg: %d -> Cfg: %d\n", g_core->set_config, new_config);
    
    /* まず、Descriptor Table 内に指定のConfig番号のConfigDescriptorがあるか確認 */
    if(new_config != 0 && get_config_desc_from_config_num(g_core, new_config) == NULL){
        PDEBUG(" --->Fail\n");
        return -1; /* 見つからない */
    }
    
#ifdef IGNORE_SAME_SET_CONFIG
    if(new_config == old_config){
        /* 現在のConfigと同じ番号でSetConfigされた時には何もしない */
        PDEBUG("SetConfig the same NowConfig: %d\n", old_config);
        
        /* EP0の通信完了後に呼ばれる関数をNULLに設定する */
        g_core->ep0_comp_func = NULL;
        
        goto comp;
    }
#endif
    
    /* 現在のConfigを終了する */
    rc = stop_config(g_core);
    if(rc != 0){
        return -1;
    }
    
    /* 新しいConfigを開始する */
    rc = start_config(g_core, new_config);
    if(rc != 0){
        return -1;
    }
    
    /* EP0の通信完了後に呼ばれる関数を設定する */
    g_core->ep0_comp_func = setup_set_config_comp;
    
comp:
    /* EP0のHalt状態を解除する */
    clear_halt_ep0(g_core);
    
    return 0;
}

static void
setup_set_config_comp(struct g_core_drv *g_core, struct usb_request *req)
{
    struct usb_kevent_arg_gadgetcore_set_config set_config;
    
    /* EP0での通信が成功していたら、USB SvcへSetConfig通知を発行する */
    if(req->status == 0){
        PDEBUG("AddQueue(SetConfig)\n");
        
        if(g_core->g_probe.hndl && g_core->g_probe.event.set_config){
            set_config.config = (unsigned char)g_core->set_config;
            usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                g_core->g_probe.event.set_config,
                                g_core->g_probe.hndl,
                                USB_KEVENT_ID_GADGETCORE_SET_CONFIG,
                                sizeof(set_config),
                                (void*)&set_config );
        }
    }
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_get_config
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    スタンダードリクエスト GetConfig を処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 *          buf       : 応答用バッファ領域
 * 出力:    0以上 : 成功(Hostへ転送するサイズ)
 *          0未満 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_get_config(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf)
{
    int rc;
    u8 config;
    
    PDEBUG("GET_CONFIG\n");
    
    /* 現在のConfig値を取得 */
    config = g_core->set_config;
    
    rc = min(ctrl->wLength, (__le16)sizeof(config));
    
    memcpy(buf, &config, rc);
    
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_set_interface
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    スタンダードリクエスト SetInterface を処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_set_interface(struct g_core_drv *g_core,
                               const struct usb_ctrlrequest *ctrl)
{
    int rc;
    u8 cfg_num, in_num, old_alt_num, new_alt_num;
    
    cfg_num = g_core->set_config;
    new_alt_num = (u8)(ctrl->wValue & 0x00ff);
    in_num  = (u8)(ctrl->wIndex & 0x00ff);
    
    if(get_alt_num(g_core, in_num, &old_alt_num) != 0){
        old_alt_num = 0xff;
    }
    
    PDEBUG("==========================================================\n");
    PDEBUG("SetInterface Cfg: %d In: %d Alt: %d -> Alt: %d \n",
              cfg_num, in_num, old_alt_num, new_alt_num);
    
    /* Configured state か確認 */
    if(cfg_num == 0){
        return -EINVAL;
    }
    
    /* DescriptorTable内に 現在のconfig値、指定のInterface値、Alt値の
       if_table_descがあるか確認 */
    if(get_if_table_desc(g_core, cfg_num, in_num, new_alt_num) == NULL){
        PDEBUG("Not Found if_table desc Cfg: %d In: %d Alt: %d\n",
                  cfg_num, in_num, new_alt_num);
        return -EINVAL;
    }
    
#ifdef IGNORE_SAME_SET_INTERFACE
    if(old_alt_num == new_alt_num){
        /* 現在のAlternateと同じ番号でSetInterfaceされた時には何もしない */
        PDEBUG("SetInterface the same NowAlternate: %d\n", old_alt_num);
        
        /* EP0の通信完了後に呼ばれる関数をNULLに設定する */
        g_core->ep0_comp_func = NULL;
        
        goto comp;
    }
#endif
    
    /* 現在のInterfaceを停止 */
    rc = stop_interface(g_core, in_num);
    if(rc){
        return -1;
    }
    
    /* 新しいAlternate番号でInterfaceを開始する */
    rc = start_interface(g_core, in_num, new_alt_num);
    if(rc){
        return -1;
    }
    
    /* Alternate管理テーブル内の指定のInterfaceのAlternate番号を変更 */
    if(set_alt_num(g_core, in_num, new_alt_num) != 0){
        PDEBUG("set_alt_num() Error\n");
        return -1;
    }
    
    /* EP0の通信完了後に呼ばれる関数を設定する */
    g_core->set_interface_info.interface_num = in_num;
    g_core->set_interface_info.alt_num = new_alt_num;
    g_core->ep0_comp_func = setup_set_interface_comp;
    
comp:
    /* EP0のHalt状態を解除する */
    clear_halt_ep0(g_core);
    
    return 0;
}

static void
setup_set_interface_comp(struct g_core_drv *g_core, struct usb_request *req)
{
    struct usb_kevent_arg_gadgetcore_set_interface set_interface;
    
    /* EP0での通信が成功していたら、USB SvcへSetConfig通知を発行する */
    if(req->status == 0){
        PDEBUG("AddQueue(SetInterface) In: %d Alt: %d\n", 
                 (unsigned char)g_core->set_interface_info.interface_num,
                 (unsigned char)g_core->set_interface_info.alt_num);
          
        if(g_core->g_probe.hndl && g_core->g_probe.event.set_interface){
            set_interface.interface =
                     (unsigned char)g_core->set_interface_info.interface_num;
            set_interface.alt = 
                     (unsigned char)g_core->set_interface_info.alt_num;
            
            usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                g_core->g_probe.event.set_interface,
                                g_core->g_probe.hndl,
                                USB_KEVENT_ID_GADGETCORE_SET_INTERFACE,
                                sizeof(set_interface),
                                (void*)&set_interface );
        }
    }
}


/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_get_interface
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    スタンダードリクエスト GetInterface を処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 *          buf       : 応答用バッファ領域
 * 出力:    0以上 : 成功(Hostへ転送するサイズ)
 *          0未満 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_get_interface(struct g_core_drv *g_core,
                               const struct usb_ctrlrequest *ctrl,
                               void *buf)
{
    u8 in_num, alt_num;
    int rc;
    
    in_num = (u8)(ctrl->wIndex & 0x00ff);
    
    PDEBUG("GetInterface(Cfg: %d In: %d)\n", g_core->set_config, in_num);
    
    /* 指定のInterfaceのAlternate番号を取得 */
    if(get_alt_num(g_core, in_num, &alt_num) != 0){
        PDEBUG(" --->Fail\n");
        return -EINVAL;
    }
    
    rc = min(ctrl->wLength, (__le16)sizeof(alt_num));
    
    memcpy(buf, &alt_num, rc);
    
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_set_feature
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    スタンダードリクエスト SetFeature を処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_set_feature(struct g_core_drv *g_core,
                             const struct usb_ctrlrequest *ctrl)
{
    usb_gadget_config_desc *cfg_desc;
    int rc;
    
    switch(ctrl->wValue){
      /**************** EP Halt ****************/
      case USB_ENDPOINT_HALT:
        if(ctrl->bRequestType != (USB_DIR_OUT       |
                                  USB_TYPE_STANDARD |
                                  USB_RECIP_ENDPOINT )){
            rc = -EINVAL;
            break;
        }
        
        PDEBUG("SetFeature(EP 0x%02x)\n", (u8)(ctrl->wIndex & 0x008f));
        
        if((ctrl->wIndex & 0x000f) == 0){
            /* EP0 SetHalt */
            PDEBUG("EP0 SetHalt\n");
            rc = set_halt_ep0(g_core);
        }else{
            /* EPn SetHalt */
            PDEBUG("EPn SetHalt\n");
            rc = set_halt_ep(g_core, (u8)(ctrl->wIndex & 0x008f));
        }
        break;
      
      /**************** DeviceRemoteWakeup ****************/
      case USB_DEVICE_REMOTE_WAKEUP:
        if(ctrl->bRequestType != (USB_DIR_OUT       |
                                  USB_TYPE_STANDARD |
                                  USB_RECIP_DEVICE   )){
            rc = -EINVAL;
            break;
        }
        PDEBUG("SetFeature(RemoteWakeup)\n");
        
        /* Config Descriptorを取得 */
        cfg_desc = get_current_config_desc(g_core);
        if(cfg_desc && (cfg_desc->uc_attributes & USB_CONFIG_ATT_WAKEUP)){
            /* 内部状態としてRemoteWakeupが許可された状態を記憶しておく */
            g_core->dev_feature |= USB_CONFIG_ATT_WAKEUP;
            rc = 0;
        }else{
            rc = -EINVAL;
        }
        
        break;
      
      /**************** TestMode ****************/
      case USB_DEVICE_TEST_MODE:
        if(ctrl->bRequestType != (USB_DIR_OUT       |
                                  USB_TYPE_STANDARD |
                                  USB_RECIP_DEVICE   )){
            rc = -EINVAL;
            break;
        }
        PDEBUG("SetFeature(TestMode)\n");
        
        if((ctrl->wIndex & 0x00ff) != 0){
            rc = -EINVAL;
            break;
        }
        
        /* テストモードを開始する */
        /* 
            Since here is too early timing for some ip,
            we start setting test mode later.
         */
#if 0
        if(start_testmode(g_core, (u8)(ctrl->wIndex >> 8)) != 0){
            rc = -EINVAL;
            break;
        }
#endif
        
        /* テストモードに入ったことを記憶する */
        g_core->test_mode = 1;
        
        rc = 0;
        break;
      
      /**************** OTG ****************/
      case USB_DEVICE_B_HNP_ENABLE:
      case USB_DEVICE_A_HNP_SUPPORT:
      case USB_DEVICE_A_ALT_HNP_SUPPORT:
        if(ctrl->bRequestType != (USB_DIR_OUT       |
                                  USB_TYPE_STANDARD |
                                  USB_RECIP_DEVICE   )){
            rc = -EINVAL;
            break;
        }
        PDEBUG("SetFeature(OTG)\n");
        
#if 0
        /* wIndex, wLengthが正しいことを確認 */
        if(ctrl->wIndex != 0 || ctrl->wLength != 0){
            rc = -EINVAL;
            break;
        }
#endif
        
        /* HNPに対応しているか確認 */
        if(!is_hnp_capable(g_core)){
            /* HNPに対応していなければSTALL応答 */
            rc = -EINVAL;
            break;
        }
        
        /* OTG CoreにSetFeatureされたことを通知 */
        usb_otgcore_gadget_set_feature(ctrl->wValue);
        
        rc = 0;
        break;
      
      default:
        rc = -EINVAL;
        break;
    }
    
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * set_halt_ep0
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    EP0に対してのSetFeatureを処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int set_halt_ep0(struct g_core_drv *g_core)
{
    g_core->ep0_halt = 1;
    
    return 0;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * clear_halt_ep0
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    EP0に対してのClearFeatureを処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int clear_halt_ep0(struct g_core_drv *g_core)
{
    g_core->ep0_halt = 0;
    
    return 0;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * set_halt_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    通常EPに対してのSetFeatureを処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int set_halt_ep(struct g_core_drv *g_core, u8 ep_adr)
{
    int rc;
    u8 cfg_num, in_num;
    unsigned char i;
    struct g_func_drv *tmp_func_drv;
    struct g_core_ep *gep;
    struct g_ep_info *ep_info;
    struct usb_ep *ep, **eps;
    
    PDEBUG("%s call\n", __func__);
    
    /* Config番号を取得 */
    cfg_num = g_core->set_config;

    //---
    // DualModeによりMTP動作に状態が遷移している場合は、
    // gcwによって動作しているfuncDrvがSICに切り替わっている。
    // よって、ここではSIC用のconfigを探し、cfg_numを上書きする
    if( (get_gcw_state() == GCW_STATE_DUAL_CONVERTING) ||
        (get_gcw_state() == GCW_STATE_DUAL_FIXED_SIC) ){
        DEBUG_PRINT("gcw State is DUAL_FIXED_SIC/DUAL_CONVERTING!");
        DEBUG_PRINT("Force To Stop SIC FuncDrv in %s!!", __FUNCTION__);

        tmp_func_drv = findFuncDrv_fromIfClasses( SIC_IF_CLASS, SIC_IF_SUBCLASS, SIC_IF_PROTOCOL);

        if( (tmp_func_drv != NULL) &&
            (tmp_func_drv->func_drv != NULL) ){
            cfg_num = tmp_func_drv->func_drv->config;
        }
        DEBUG_INFO("SICs cfg_num is %d", cfg_num);
    }
    else{
        DEBUG_PRINT("gcw State is NOT DUAL_FIXED_SIC/DUAL_CONVERTING.");
        DEBUG_PRINT("So, will not stop SIC here in %s", __FUNCTION__);
    }
    //---
    PDEBUG("cfg_num: %d\n", cfg_num);

    /* EPアドレスからInterface番号を取得 */
    in_num = ep_ctrl_get_in_num(g_core, ep_adr);
    if(in_num == USB_GCORE_INT_NOT_DEF){
        PDEBUG("ep_adr not found\n");
        rc = -EINVAL;
        goto exit;
    }
    PDEBUG("in_num: %d\n", cfg_num);

    /* FuncDrvの中から該当するInterfaceで動いているFuncDrvを探す */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->config == cfg_num   &&
           tmp_func_drv->func_drv->interface == in_num &&
           tmp_func_drv->func_drv->ep_set_halt != NULL    ){
            
            eps = tmp_func_drv->ep_list.ep;
            PDEBUG("eps: %lx\n", (unsigned long)eps);
            
            for(i = 0; i < tmp_func_drv->ep_list.num_ep; i++){
                ep = *(eps + i);
                PDEBUG("ep: %lx\n", (unsigned long)ep);
                gep = container_of(ep, struct g_core_ep, ep);
                PDEBUG("gep: %lx\n", (unsigned long)gep);
                ep_info = (struct g_ep_info*)gep->_ep->driver_data;
                PDEBUG("ep_info: %lx\n", (unsigned long)ep_info);
                PDEBUG("ep_info->use: %d ep_info->ep_adr: %x\n", ep_info->use, ep_info->ep_adr);
                
                /* EPが使用中 かつ EPアドレスが一致 か確認 */
                if(ep_info->use &&
                   ep_info->ep_adr == ep_adr){
                    
                    /* FunctionDriverのep_st_halt() を呼ぶ */
                    PDEBUG("ep_set_halt()\n");
                    rc = tmp_func_drv->func_drv->ep_set_halt(tmp_func_drv->func_drv,
                                                             ep);
                    if(rc == 0){
                        /* 0を返すFuncDrvがあればそこで終了 */
                        /* StatusStageへ進む */
                        rc = 0;
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALLを返すFuncDrvがあればそこで終了 */
                        /* EP0をProtocolStallにする */
                        rc = -EINVAL;
                        goto exit;
                    }
                }
            }
        }
    }
    
    /* 処理をするFuncDrvが見つからない場合には、EP0をProtocolStallにする */
    rc = -EINVAL;
exit:
    return rc;
}


/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * clear_halt_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    通常EPに対してのClearFeatureを処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int clear_halt_ep(struct g_core_drv *g_core, u8 ep_adr)
{
    int rc;
    u8 cfg_num, in_num;
    unsigned char i;
    struct g_func_drv *tmp_func_drv;
    struct g_core_ep *gep;
    struct g_ep_info *ep_info;
    struct usb_ep *ep, **eps;
    
    PDEBUG("%s call\n", __func__);
    
    /* Config番号を取得 */
    cfg_num = g_core->set_config;

    //---
    // DualModeによりMTP動作に状態が遷移している場合は、
    // gcwによって動作しているfuncDrvがSICに切り替わっている。
    // よって、ここではSIC用のconfigを探し、cfg_numを上書きする
    if( (get_gcw_state() == GCW_STATE_DUAL_CONVERTING) ||
        (get_gcw_state() == GCW_STATE_DUAL_FIXED_SIC) ){
        DEBUG_PRINT("gcw State is DUAL_FIXED_SIC/DUAL_CONVERTING!");
        DEBUG_PRINT("Force To Stop SIC FuncDrv in %s!!", __FUNCTION__);

        tmp_func_drv = findFuncDrv_fromIfClasses( SIC_IF_CLASS, SIC_IF_SUBCLASS, SIC_IF_PROTOCOL);

        if( (tmp_func_drv != NULL) &&
            (tmp_func_drv->func_drv != NULL) ){
            cfg_num = tmp_func_drv->func_drv->config;
        }
        DEBUG_INFO("SICs cfg_num is %d", cfg_num);
    }
    else{
        DEBUG_PRINT("gcw State is NOT DUAL_FIXED_SIC/DUAL_CONVERTING.");
        DEBUG_PRINT("So, will not stop SIC here in %s", __FUNCTION__);
    }
    //---
    PDEBUG("cfg_num: %d\n", cfg_num);

    /* EPアドレスからInterface番号を取得 */
    in_num = ep_ctrl_get_in_num(g_core, ep_adr);
    if(in_num == USB_GCORE_INT_NOT_DEF){
        PDEBUG("ep_adr not found\n");
        rc = -EINVAL;
        goto exit;
    }
    PDEBUG("in_num: %d\n", cfg_num);
    PDEBUG("ep_adr: %x\n", ep_adr);

    /* FuncDrvの中から該当するInterfaceで動いているFuncDrvを探す */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->config == cfg_num   &&
           tmp_func_drv->func_drv->interface == in_num &&
           tmp_func_drv->func_drv->ep_clear_halt != NULL    ){
            
            eps = tmp_func_drv->ep_list.ep;
            PDEBUG("eps: 0x%lx\n", (unsigned long)eps);
            
            for(i = 0; i < tmp_func_drv->ep_list.num_ep; i++){
                ep = *(eps + i);
                PDEBUG("ep: 0x%lx\n", (unsigned long)ep);
                gep = container_of(ep, struct g_core_ep, ep);
                PDEBUG("gep: 0x%lx\n", (unsigned long)gep);
                ep_info = (struct g_ep_info*)gep->_ep->driver_data;
                PDEBUG("ep_info: 0x%lx\n", (unsigned long)ep_info);
                PDEBUG("ep_info->use: %d ep_info->ep_adr: %x\n", ep_info->use, ep_info->ep_adr);
                
                /* EPが使用中 かつ EPアドレスが一致 か確認 */
                if(ep_info->use &&
                   ep_info->ep_adr == ep_adr){
                    
                    /* FunctionDriverのep_st_halt() を呼ぶ */
                    PDEBUG("ep_clear_halt()\n");
                    rc = tmp_func_drv->func_drv->ep_clear_halt(tmp_func_drv->func_drv,
                                                               ep);
                    if(rc == 0){
                        /* 0を返すFuncDrvがあればそこで終了 */
                        /* StatusStageへ進む */
                        rc = 0;
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALLを返すFuncDrvがあればそこで終了 */
                        /* EP0をProtocolStallにする */
                        rc = -EINVAL;
                        goto exit;
                    }
                }
            }
        }
    }
    
    /* 処理をするFuncDrvが見つからない場合には、EP0をProtocolStallにする */
    rc = -EINVAL;
exit:
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * get_status_ep0
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    EP0のGetStatusを処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 * 出力:    
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int get_status_ep0(struct g_core_drv *g_core,
                          const struct usb_ctrlrequest *ctrl,
                          void *buf)
{
    int rc;
    u16 status;
    
    /* EP0のHalt状態を取得する */
    rc = ep_ctrl_get_ep0_halt(g_core);
    status = 0;
    if(rc != 0){
        status = 1;
    }
    
    rc = min(ctrl->wLength, (__le16)sizeof(status));
    memcpy(buf, &status, rc);
    
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * get_status_ep
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    EPnのGetStatusを処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ep_adr    : EP Address
 * 出力:    
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int get_status_ep(struct g_core_drv *g_core,
                         const struct usb_ctrlrequest *ctrl,
                         void *buf)
{
    u16 status;
    int rc;
    
    /* Host指定のEPアドレスのStall状態を取得する */
    rc = ep_ctrl_get_ep_stall(g_core, ctrl->wIndex & 0x008f);
    if(rc > 0){
        /* Stall */
        status = 1;
    }else if(rc == 0){
        /* Not Stall */
        status = 0;
    }else{
        /* 該当するEP見つからない -> EP0 Protocol Error */
        return -1;
    }
    
    rc = min(ctrl->wLength, (__le16)sizeof(status));
    memcpy(buf, &status, rc);
    
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_clear_feature
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    スタンダードリクエスト ClearFeature を処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_clear_feature(struct g_core_drv *g_core,
                               const struct usb_ctrlrequest *ctrl)
{
    usb_gadget_config_desc *cfg_desc;
    int rc;
    
    switch(ctrl->wValue){
      /**************** EP Halt ****************/
      case USB_ENDPOINT_HALT:
        if(ctrl->bRequestType != (USB_DIR_OUT       |
                                  USB_TYPE_STANDARD |
                                  USB_RECIP_ENDPOINT )){
            rc = -EINVAL;
            break;
        }
        
        PDEBUG("ClearFeature(EP 0x%02x)\n", (u8)(ctrl->wIndex & 0x008f));
        
        if((ctrl->wIndex & 0x000f) == 0){
            /* EP0 SetHalt */
            PDEBUG("EP0 SetHalt\n");
            rc = clear_halt_ep0(g_core);
        }else{
            /* EPn SetHalt */
            PDEBUG("EPn SetHalt\n");
            rc = clear_halt_ep(g_core, (u8)(ctrl->wIndex & 0x008f));
        }
        break;
      
      /**************** DeviceRemoteWakeup ****************/
      case USB_DEVICE_REMOTE_WAKEUP:
        if(ctrl->bRequestType != (USB_DIR_OUT       |
                                  USB_TYPE_STANDARD |
                                  USB_RECIP_DEVICE   )){
            rc = -EINVAL;
            break;
        }
        PDEBUG("ClearFeature(RemoteWakeup)\n");
        
        /* Config Descriptorを取得 */
        cfg_desc = get_current_config_desc(g_core);
        if(cfg_desc && (cfg_desc->uc_attributes & USB_CONFIG_ATT_WAKEUP)){
            /* 内部状態としてRemoteWakeupが許可された状態をクリアしておく */
            g_core->dev_feature &= ~USB_CONFIG_ATT_WAKEUP;
            rc = 0;
        }else{
            rc = -EINVAL;
        }
        
        break;
      
      default:
        rc = -EINVAL;
        break;
    }
    
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_get_status
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    スタンダードリクエスト GetStatus を処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 *          buf       : 応答用バッファ領域
 * 出力:    0以上 : 成功(Hostへ転送するサイズ)
 *          0未満 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_get_status(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf)
{
    usb_gadget_config_desc *cfg_desc;
    u8  i;
    int rc;
    u16 status;
    
    /* wValue が0かを確認 */
    if(ctrl->wValue != 0){
        return -EINVAL;
    }
    
    if(ctrl->bRequestType == (USB_DIR_IN        |
                              USB_TYPE_STANDARD |
                              USB_RECIP_DEVICE   )){
        PDEBUG("GetStatus(Device)\n");
        
        status = 0;
        
        /* SelfPowerを設定 */
        if(g_core->set_config == 0){
            /* Address状態ならばDefault値を使用 */
            
            /* SpeedDescのuc_default_attributesのSelf-Powerdフラグを確認 */
            if(g_core->desc_tbl->uc_default_attributes & USB_CONFIG_ATT_SELFPOWER){
                status |= (1 << USB_DEVICE_SELF_POWERED);
            }
        }else{
            /* Configured状態ならばConfigDescの値を使用 */
            
            cfg_desc = get_current_config_desc(g_core);
            if(cfg_desc == NULL){
                rc = -EINVAL;
                goto end;
            }
            
            /* ConfigDescのuc_attributesのSelf-Powerdフラグを確認 */
            if(cfg_desc->uc_attributes & USB_CONFIG_ATT_SELFPOWER){
                status |= (1 << USB_DEVICE_SELF_POWERED);
            }
        }
        
        /* RemoteWakeup許可状態を設定 */
        if(g_core->dev_feature & USB_CONFIG_ATT_WAKEUP){
            status |= (1 << USB_DEVICE_REMOTE_WAKEUP);
        }
        
        status = cpu_to_le16(status);
        rc = min(ctrl->wLength, (u16)sizeof(status));
        memcpy(buf, &status, rc);
    }else if(ctrl->bRequestType == (USB_DIR_IN        |
                                    USB_TYPE_STANDARD |
                                    USB_RECIP_ENDPOINT )){
        if((ctrl->wIndex & 0x000f) == 0){
            /* EP0 */
            PDEBUG("GetStatus(EP0)\n");
            rc = get_status_ep0(g_core, ctrl, buf);
        }else{
            /* EP0 以外 */
            PDEBUG("GetStatus(EPn)\n");
            rc = get_status_ep(g_core, ctrl, buf);
        }
        
    }else if(ctrl->bRequestType == (USB_DIR_IN         |
                                    USB_TYPE_STANDARD  |
                                    USB_RECIP_INTERFACE )){
        PDEBUG("GetStatus(Interface)\n");
        
        if(g_core->set_config == 0){
            /* Address状態ならばStallする */
            rc = -EINVAL;
            goto end;
        }
        
        /* 現在のConfigDescriptorを求める */
        cfg_desc = get_current_config_desc(g_core);
        if(cfg_desc == NULL){
            rc = -EINVAL;
            goto end;
            
        }
        
        /* 指定のInterfaceが存在するか探す */
        for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
            if(cfg_desc->interfaces[i].uc_if_number == (ctrl->wIndex & 0x00ff)){
                break;
            }
        }
        if(i == cfg_desc->uc_num_interfaces){
            rc = -EINVAL;
            goto end;
        }
        
        status = 0;
        status = cpu_to_le16(status);
        rc = min(ctrl->wLength, (u16)sizeof(status));
        memcpy(buf, &status, rc);
    }else{
        rc = -EINVAL;
    }
    
end:
    return rc;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_get_ms_descriptor
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    リクエスト GetMSDescriptor を処理する
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 *          buf       : 応答用バッファ領域
 * 出力:    0以上 : 成功(Hostへ転送するサイズ)
 *          0未満 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
static int setup_get_ms_descriptor(struct g_core_drv *g_core,
                                   const struct usb_ctrlrequest *ctrl,
                                   void *buf)
{
    int rc;

    /* Check inputs for get_ms_descriptor */
    if(ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE   )){
        DEBUG_ERR("invalid bRequestType for %s[0x%x]", __func__, ctrl->bRequestType);
        return -EINVAL;
    }
    if(ctrl->bRequest != USB_FEATURE_DESCRIPTOR_CODE ){
        DEBUG_ERR("invalid bReqeust for %s[0x%x]", __func__, ctrl->bRequest);
        return -EINVAL;
    }
    if(ctrl->wValue != ( (USB_REQ_FEATURE_DESC_WVALUE_H*0x0100) | (USB_REQ_FEATURE_DESC_WVALUE_L) ) ){
        DEBUG_ERR("invalid wValue for %s[0x%x]", __func__, ctrl->wValue);
        return -EINVAL;
    }
    if(ctrl->wIndex != USB_REQ_FEATURE_DESC_WINDEX ){
        DEBUG_ERR("invalid wIndex for %s[0x%x]", __func__, ctrl->wIndex);
        return -EINVAL;
    }

    /* copy get_ms_descriptor response to buffer */
    if( g_core->enable_dual_mode != 0 ){
        rc = min( ctrl->wLength, (u16)sizeof(g_ext_conf_desc) );
        DEBUG_INFO("now in setup_get_ms_descriptor!!, rc = 0x%x", rc);
        memcpy(buf, &g_ext_conf_desc, rc);
    }
    else{
        DEBUG_ERR("## now in setup_get_ms_descriptor!!, but dual_mode is disabled!!!");
        return -EINVAL;
    }

    return rc;
}

#define USB_DT_HID    ( 0x21 )  // For HID Descriptor.
#define USB_DT_REPORT ( 0x22 )  // For Report Descriptor.

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * setup_req
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    SETUPを受けたときに呼ばれる
 * 入力:    g_core    : GadgetCoreへのポインタ
 *          ctrl      : SETUPパケットデータ
 * 出力:    0以上 : 成功(Hostへ転送するサイズ)
 *          0未満 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
int
setup_req(struct g_core_drv *g_core, const struct usb_ctrlrequest *ctrl)
{
    struct usb_request      *req = g_core->ep0req;
    int rc = -EINVAL;
    u8 current_config;
    
    if((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD){
        PDEBUG("STANDARD_REQUEST\n");
        
        switch(ctrl->bRequest){
          case USB_REQ_GET_DESCRIPTOR:
          
            /* EP0がHalt中ならばSTALLする */
            if(ep_ctrl_get_ep0_halt(g_core) != 0){
                rc = -EINVAL;
                break;
            }
            
            PDEBUG("GET_DESCRIPTOR - ");
            
            if(  ( ctrl->bRequestType != ( USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE    ) )
              && ( ctrl->bRequestType != ( USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE ) )
              )
            {
                PDEBUG("ERROR\n");
                break;
            }
            
            switch(ctrl->wValue >> 8){
              case USB_DT_DEVICE:
                PDEBUG("DEVICE\n");
                
                rc = make_device_desc(g_core, ctrl, req->buf);
                
                break;
                
              case USB_DT_DEVICE_QUALIFIER:
                PDEBUG("QUALIFIER\n");
                
                if(g_core->hs_disable == USB_OTGCORE_HS_DISABLE){
                    /* FS固定モードならばSTALLする */
                    rc = -EINVAL;
                }else{
                    rc = make_device_qualifier_desc(g_core, ctrl, req->buf);
                }
                
                break;
                
              case USB_DT_CONFIG:
                PDEBUG("CONFIG\n");
                
                rc = make_config_desc(g_core, ctrl, req->buf, USB_DT_CONFIG);
                
                break;
                
              case USB_DT_OTHER_SPEED_CONFIG:
                PDEBUG("OTHER_SPEED\n");
                
                if(g_core->hs_disable == USB_OTGCORE_HS_DISABLE){
                    /* FS固定モードならばSTALLする */
                    rc = -EINVAL;
                }else{
                    rc = make_config_desc(g_core, ctrl, req->buf, USB_DT_OTHER_SPEED_CONFIG);
                }
                
                break;
                
              case USB_DT_STRING:
                PDEBUG("STRING\n");
                
                rc = make_string_desc(g_core, ctrl, req->buf);
                
                break;
                
              case USB_DT_HID:
                PDEBUG( "HID_DESCRIPTOR\n" );
                
                rc = make_hid_desc( g_core, ctrl, req->buf );
                
                break;
                
              case USB_DT_REPORT:
                PDEBUG( "GET_REPORT_DESCRIPTOR received. \n" );
                
                rc = make_report_desc( g_core, ctrl, req->buf );
                
                break;
                
              default:
                PDEBUG( "ERROR ctrl->wValue is 0x%x \n", ( ctrl->wValue >> 8 ) );
                break;
            }
            break;
            
          case USB_REQ_SET_CONFIGURATION:
            rc = setup_set_config(g_core, ctrl);
            
            if( rc >= 0 ){
                current_config = g_core->set_config;
                usb_gcw_setup_setconfig( current_config );
            }
            else{
                DEBUG_ERR("rc for setup_set_config() = %d", rc);
            }
            break;
            
          case USB_REQ_GET_CONFIGURATION:
            /* EP0がHalt中ならばSTALLする */
            if(ep_ctrl_get_ep0_halt(g_core) != 0){
                rc = -EINVAL;
                break;
            }
            
            rc = setup_get_config(g_core, ctrl, req->buf);
            break;
            
          case USB_REQ_SET_INTERFACE:
            rc = setup_set_interface(g_core, ctrl);
            
            if( rc >= 0 ){
                current_config = g_core->set_config;
                usb_gcw_setup_setinterface( current_config );
            }
            else{
                DEBUG_ERR("rc for setup_set_interface() = %d", rc);
            }
            break;
            
          case USB_REQ_GET_INTERFACE:
            /* EP0がHalt中ならばSTALLする */
            if(ep_ctrl_get_ep0_halt(g_core) != 0){
                rc = -EINVAL;
                break;
            }
            
            rc = setup_get_interface(g_core, ctrl, req->buf);
            break;
            
          
          case USB_REQ_SET_FEATURE:
            PDEBUG("SET_FEATURE\n");
            
            rc = setup_set_feature(g_core, ctrl);
            
            break;
            
          case USB_REQ_CLEAR_FEATURE:
            PDEBUG("CLEAR_FEATURE\n");
            
            rc = setup_clear_feature(g_core, ctrl);
            
            break;
          
          case USB_REQ_GET_STATUS:
            PDEBUG("GET_STATUS\n");
            
            rc = setup_get_status(g_core, ctrl, req->buf);
            
            break;
            
          default:
            break;
        }
        
        if(rc >= 0){
            g_core->ep0req->length = rc;
            g_core->ep0req->complete = ep0_complete;
            g_core->ep0req->zero = (rc < ctrl->wLength &&
                                    (rc % g_core->gadget->ep0->maxpacket) == 0);
            g_core->ep0->driver_data = g_core;
            
            PDEBUG("usb_ep_queue(EP0)\n");
            PVERBOSE(" buf:    %lx\n", (unsigned long)g_core->ep0req->buf);
            PVERBOSE(" length: %d\n", g_core->ep0req->length);
            PVERBOSE(" dma: %d\n", (unsigned int)g_core->ep0req->dma);
            PVERBOSE(" no_interrupt: %d\n", g_core->ep0req->no_interrupt);
            PVERBOSE(" zero: %d\n", g_core->ep0req->zero);
            PVERBOSE(" short_not_ok: %d\n", g_core->ep0req->short_not_ok);
            
            rc = usb_ep_queue(g_core->ep0, g_core->ep0req, GFP_ATOMIC);
        }
        
        /* Start test mode. */
        if( g_core->test_mode ){
            
            PDEBUG("Now is the test mode! \n");
            start_testmode( g_core, (u8)( ctrl->wIndex >> 8 ) );
            
        }
        
    }else if((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS){
        PDEBUG("CLASS SPECIFIC REQUEST\n");
        
        rc = setup_class_specific(g_core, ctrl);
        
    }else if((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR){
        PDEBUG("VENDOR SPECIFIC REQUEST\n");
        
        rc = setup_vendor_specific(g_core, ctrl);
        
    }
    
    return rc;
}

