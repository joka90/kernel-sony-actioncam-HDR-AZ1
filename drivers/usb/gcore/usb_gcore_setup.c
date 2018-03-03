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

/*����������������������������������������������������������������������*
 * ep0_complete
 *����������������������������������������������������������������������*
 * ����:    GadgetCore����ư�ǽ�������EP0�̿��δ�λCallback��
 *          usb_ep_queue(ep0, ep0req) �ƤӽФ����� ep0req->complete������
 * ����:    *ep        : �̿���λ����EP�ؤΥݥ���
 *          *req       : �̿���λ����request�ؤΥݥ���
 * ����:    
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static void
ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct g_core_drv *g_core;
    
    PDEBUG("ep0_complete()\n");
    
    /* EP0��driver_data���� g_core�ؤΥݥ��󥿤���� */
    g_core = ep->driver_data;
    if(!g_core) return;
    
    /* EP0�Ǥ��̿�����λ������˸Ƥִؿ�����Ͽ����Ƥ���иƤ� */
    if(g_core->ep0_comp_func){
        PDEBUG("    ep0_comp_func()\n");
        g_core->ep0_comp_func(g_core, req);
        
        /* EP0�Ǥ��̿�����λ������˸Ƥִؿ����˴����� */
        g_core->ep0_comp_func = NULL;
    }
}

/*����������������������������������������������������������������������*
 * setup_class_specific
 *����������������������������������������������������������������������*
 * ����:    ClassSpecificRequest��FuncDrv�����Τ���
 * ����:    g_core     : GadgetCore�ؤΥݥ���
 *          ctrl       : SETUP�ѥ��å�
 * ����:     0 : ����
 *           0 : STALL
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int setup_class_specific(struct g_core_drv *g_core,
                                const struct usb_ctrlrequest *ctrl)
{
    struct g_func_drv *tmp_func_drv;
    u8 cfg_num, in_num;
    int rc;
    
    if((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE){
        /* ������Interface����� */
        in_num = (u8)(ctrl->wIndex & 0x00ff);
        cfg_num = g_core->set_config;
        PDEBUG("ClassSpecificRequest(INTERFACE) In: %d\n", in_num);

        //---
        // DualMode�ˤ��MTPư��˾��֤����ܤ��Ƥ�����ϡ�
        // gcw�ˤ�ä�ư��Ƥ���funcDrv��SIC���ڤ��ؤ�äƤ��롣
        // ��äơ������Ǥ�SIC�Ѥ�config��õ����cfg_num���񤭤���
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

        /* ��������FuncDrv��õ����class()��Ƥ� */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->config    == cfg_num &&
               tmp_func_drv->func_drv->interface == in_num     ){
                /* �����Ǿ��ΰ��פ���FuncDrv��start()���Τ����� */
                if(tmp_func_drv->func_drv->class){
                    PVERBOSE(" class() to %s\n", 
                             tmp_func_drv->func_drv->function ?
                             tmp_func_drv->func_drv->function : "");
                    rc = tmp_func_drv->func_drv->class(tmp_func_drv->func_drv,
                                                          ctrl, g_core->ep0);
                    if(rc == 0){
                        /* 0���֤�FuncDrv������Ф����ǽ�λ */
                        /* EP0��Halt���֤������� */
                        clear_halt_ep0(g_core);
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALL���֤�FuncDrv������Ф����ǽ�λ */
                        PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                        rc = -EINVAL;
                        goto exit;
                    }
                    PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
                }
            }
        }
        /* �ɤ�FuncDrv������򤷤ʤ�����Stall�ˤ��� */
        rc = -EINVAL;
    }else if((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT){
        /* ���ꤵ�줿ep_adr��°����Cfg�ֹ����� */
        cfg_num = g_core->set_config;

        //---
        // DualMode�ˤ��MTPư��˾��֤����ܤ��Ƥ�����ϡ�
        // gcw�ˤ�ä�ư��Ƥ���funcDrv��SIC���ڤ��ؤ�äƤ��롣
        // ��äơ������Ǥ�SIC�Ѥ�config��õ����cfg_num���񤭤���
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

        /* ���ꤵ�줿ep_adr��°����In�ֹ����� */
        in_num = ep_ctrl_get_in_num(g_core, (ctrl->wIndex & 0x00ff));
        if(in_num == USB_GCORE_INT_NOT_DEF){
            PDEBUG("ep_adr not found\n");
            rc = -EINVAL;
            goto exit;
        }

        /* ��������FuncDrv��õ����class()��Ƥ� */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->config    == cfg_num &&
               tmp_func_drv->func_drv->interface == in_num     ){
                /* �����Ǿ��ΰ��פ���FuncDrv��start()���Τ����� */
                if(tmp_func_drv->func_drv->class){
                    PVERBOSE(" class() to %s\n", 
                             tmp_func_drv->func_drv->function ?
                             tmp_func_drv->func_drv->function : "");
                    rc = tmp_func_drv->func_drv->class(tmp_func_drv->func_drv,
                                                          ctrl, g_core->ep0);
                    if(rc == 0){
                        /* 0���֤�FuncDrv������Ф����ǽ�λ */
                        /* EP0��Halt���֤������� */
                        clear_halt_ep0(g_core);
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALL���֤�FuncDrv������Ф����ǽ�λ */
                        PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                        rc = -EINVAL;
                        goto exit;
                    }
                    PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
                }
            }
        }
        /* �ɤ�FuncDrv������򤷤ʤ�����Stall�ˤ��� */
        rc = -EINVAL;
    }else{   /* Device,Other�� */
        /* ��������FuncDrv��õ����class()��Ƥ� */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->class){
                PVERBOSE(" class() to %s\n", 
                         tmp_func_drv->func_drv->function ?
                         tmp_func_drv->func_drv->function : "");
                rc = tmp_func_drv->func_drv->class(tmp_func_drv->func_drv,
                                                      ctrl, g_core->ep0);
                if(rc == 0){
                    /* 0���֤�FuncDrv������Ф����ǽ�λ */
                    /* EP0��Halt���֤������� */
                    clear_halt_ep0(g_core);
                    goto exit;
                }else if(rc == USB_GADGETCORE_STALL){
                    /* STALL���֤�FuncDrv������Ф����ǽ�λ */
                    PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                    rc = -EINVAL;
                    goto exit;
                }
                PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
            }
        }
        /* �ɤ�FuncDrv������򤷤ʤ�����Stall�ˤ��� */
        rc = -EINVAL;
    }
    
exit:
    return rc;
}

/*����������������������������������������������������������������������*
 * setup_vendor_specific
 *����������������������������������������������������������������������*
 * ����:    ClassSpecificRequest��FuncDrv��ʬ�ۤ���
 * ����:    g_core     : GadgetCore�ؤΥݥ���
 *          ctrl       : SETUP�ѥ��å�
 * ����:     0 : ����
 *           0 : STALL
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int setup_vendor_specific(struct g_core_drv *g_core,
                                 const struct usb_ctrlrequest *ctrl)
{
    struct g_func_drv *tmp_func_drv;
    u8 cfg_num, in_num;
    int rc;
    
    if((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE){
        /* ������Interface����� */
        in_num = (u8)(ctrl->wIndex & 0x00ff);
        cfg_num = g_core->set_config;
        PDEBUG("VendorSpecificRequest(INTERFACE) In: %d\n", in_num);

        /* ��������FuncDrv��õ����vendor()��Ƥ� */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->config    == cfg_num &&
               tmp_func_drv->func_drv->interface == in_num     ){
                /* �����Ǿ��ΰ��פ���FuncDrv��start()���Τ����� */
                if(tmp_func_drv->func_drv->vendor){
                    PVERBOSE(" vendor() to %s\n", 
                             tmp_func_drv->func_drv->function ?
                             tmp_func_drv->func_drv->function : "");
                    rc = tmp_func_drv->func_drv->vendor(tmp_func_drv->func_drv,
                                                           ctrl, g_core->ep0);
                    if(rc == 0){
                        /* 0���֤�FuncDrv������Ф����ǽ�λ */
                        /* EP0��Halt���֤������� */
                        clear_halt_ep0(g_core);
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALL���֤�FuncDrv������Ф����ǽ�λ */
                        PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                        rc = -EINVAL;
                        goto exit;
                    }
                    PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
                }
            }
        }
        /* �ɤ�FuncDrv������򤷤ʤ�����Stall�ˤ��� */
        rc = -EINVAL;
    }else if((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT){
        /* ���ꤵ�줿ep_adr��°����Cfg,In�ֹ����� */
        cfg_num = g_core->set_config;
        
        in_num = ep_ctrl_get_in_num(g_core, (ctrl->wIndex & 0x00ff));
        if(in_num == USB_GCORE_INT_NOT_DEF){
            PDEBUG("ep_adr not found\n");
            rc = -EINVAL;
            goto exit;
        }
        
        /* ��������FuncDrv��õ����vendor()��Ƥ� */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->config    == cfg_num &&
               tmp_func_drv->func_drv->interface == in_num     ){
                /* �����Ǿ��ΰ��פ���FuncDrv��start()���Τ����� */
                if(tmp_func_drv->func_drv->vendor){
                    PVERBOSE(" vendor() to %s\n", 
                             tmp_func_drv->func_drv->function ?
                             tmp_func_drv->func_drv->function : "");
                    rc = tmp_func_drv->func_drv->vendor(tmp_func_drv->func_drv,
                                                           ctrl, g_core->ep0);
                    if(rc == 0){
                        /* 0���֤�FuncDrv������Ф����ǽ�λ */
                        /* EP0��Halt���֤������� */
                        clear_halt_ep0(g_core);
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALL���֤�FuncDrv������Ф����ǽ�λ */
                        PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                        rc = -EINVAL;
                        goto exit;
                    }
                    PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
                }
            }
        }
        /* �ɤ�FuncDrv������򤷤ʤ�����Stall�ˤ��� */
        rc = -EINVAL;
    }else{   /* Device,Other�� */

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

        /* ��������FuncDrv��õ����vendor()��Ƥ� */
        list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
            if(tmp_func_drv->func_drv->vendor){
                PVERBOSE(" vendor() to %s\n", 
                         tmp_func_drv->func_drv->function ?
                         tmp_func_drv->func_drv->function : "");
                rc = tmp_func_drv->func_drv->vendor(tmp_func_drv->func_drv,
                                                       ctrl, g_core->ep0);
                if(rc == 0){
                    /* 0���֤�FuncDrv������Ф����ǽ�λ */
                    /* EP0��Halt���֤������� */
                    clear_halt_ep0(g_core);
                    goto exit;
                }else if(rc == USB_GADGETCORE_STALL){
                    /* STALL���֤�FuncDrv������Ф����ǽ�λ */
                    PVERBOSE("  --->rc: USB_GADGETCORE_STALL\n");
                    rc = -EINVAL;
                    goto exit;
                }
                PVERBOSE("  --->rc: USB_GADGETCORE_THRU\n");
            }
        }
        /* �ɤ�FuncDrv������򤷤ʤ�����Stall�ˤ��� */
        rc = -EINVAL;
    }
    
exit:
    return rc;
}

/*����������������������������������������������������������������������*
 * start_config
 *����������������������������������������������������������������������*
 * ����:    �����Config��ư���ep��enable�ˤ���
 *          Config��Interface�����פ���FuncDrv��start()�����Τ���
 * ����:    g_core     : GadgetCore�ؤΥݥ���
 *          config_num : Config�ֹ�
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int start_config(struct g_core_drv *g_core, u8 config_num)
{
    usb_gadget_config_desc *cfg_desc;
    int rc;
    u8 i;
    
    /* Config�ͤ�0�ʤ���ͤ�����¸���ƽ�λ */
    if(config_num == 0){
        g_core->set_config = 0;
        return 0;
    }
    
    /* Descriptor Table ��������Config�ֹ��ConfigDescriptor����� */
    cfg_desc = get_config_desc_from_config_num(g_core, config_num);
    if(cfg_desc == NULL){
        return -1; /* ���Ĥ���ʤ� */
    }
    
    /* FIFO�����ꤹ�� */
    rc = set_fifo_map(g_core, cfg_desc);
    if(rc != 0){
        return -1;
    }
    
    /* ������Config�ͤȤ�����¸ */
    g_core->set_config = config_num;
    
    /* Alternate�ֹ���¸�ΰ������ */
    make_alt_num_list(g_core);
    
    /* Config���Interface���������� */
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        rc = start_interface(g_core, cfg_desc->interfaces[i].uc_if_number, 0);
        if(rc){
            PDEBUG("start_config() Error\n");
            return -1;
        }
    }
    
    return 0;
}

/*����������������������������������������������������������������������*
 * setup_disconnect
 *����������������������������������������������������������������������*
 * ����:    disconnect���줿���ν���
 * ����:    g_core     : GadgetCore�ؤΥݥ���
 * ����:    
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
void
setup_disconnect(struct g_core_drv *g_core)
{
    
    unsigned long flags;
    
    //lock
    spin_lock_irqsave(&g_core->lock_stop_config, flags);
    
    /* ����Config����� */
    stop_config(g_core);
    
    /* EP0��halt���֤��� */
    clear_halt_ep0(g_core);
    
    //unlock
    spin_unlock_irqrestore(&g_core->lock_stop_config, flags);
    
}

/*����������������������������������������������������������������������*
 * stop_config
 *����������������������������������������������������������������������*
 * ����:    ������Config�ֹ��ư���Ƥ���FuncDrv��stop()�����Τ�
 *          ep������disable�ˤ���
 * ����:    g_core     : GadgetCore�ؤΥݥ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int stop_config(struct g_core_drv *g_core)
{
    usb_gadget_config_desc *cfg_desc;
    int rc;
    u8 i;
    
    /* ���ߤ�Config�ͤ�0�ʤ�в��⤷�ʤ� */
    if(g_core->set_config == 0){
        return 0;
    }
    
    /* ���ߤ�ConfigDescriptor��������� */
    cfg_desc = get_config_desc_from_config_num(g_core, g_core->set_config);
    if(cfg_desc == NULL){
        PDEBUG(" --->Fail\n");
        return -1; /* ���Ĥ���ʤ� */
    }
    
    /* Alternate�ֹ���¸�ΰ���� */
    free_alt_num_list(g_core);
    
    /* Config���Interface��λ���� */
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        rc = stop_interface(g_core, cfg_desc->interfaces[i].uc_if_number);
        if(rc != 0){
            PDEBUG("stop_config() Error\n");
            return -1;
        }
    }
    
    /* Config�ͤ�0�ˤ��� */
    g_core->set_config = 0;
    
    /* RemoteWakeup���ľ��֤򥯥ꥢ */
    g_core->dev_feature = 0;
    
    /* FIFO�������ꥻ�åȤ��� */
    PVERBOSE("usb_gadget_ioctl(USB_IOCTL_FIFO_RESET)\n");
    rc = usb_gadget_ioctl(g_core->gadget, USB_IOCTL_FIFO_RESET, 0);
    if(rc != 0){
        PDEBUG("usb_gadget_ioctl(USB_IOCTL_FIFO_RESET) Error\n");
        return -1;
    }
    
    return 0;
}


/*����������������������������������������������������������������������*
 * set_fifo_map
 *����������������������������������������������������������������������*
 * ����:    ������ä�Config Descriptor����FIFO�ΰ�����ꤹ��
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          cfg_desc  : ���ߤ�Speed��Confi�ֹ�� Config Descriptor �ؤΥݥ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int set_fifo_map(struct g_core_drv *g_core, usb_gadget_config_desc *cfg_desc)
{
    usb_gadget_interface_desc *if_desc;
    usb_gadget_if_table *setting;
    usb_gadget_if_table_element *ift_element;
    struct usb_ioctl_fifo_param fifo_param;
    int rc;
    u8 ep_num;  /* EP�ֹ�(EP���ɥ쥹�ǤϤʤ�) */
    u8 ep_adr = 0;  /* EP���ɥ쥹 (EP�ֹ� + Direction) */
    u8 type = 0;
    u16 max_mps = 0;
    u8 int_num, exist_ep;
    u8 i, j, k;
    
    PVERBOSE("----------------------------------\n");
    PVERBOSE("set_fifo_map()\n");
    
    /* ǰ�Τ����FIFO�򥯥ꥢ���� */
    rc = usb_gadget_ioctl(g_core->gadget, USB_IOCTL_FIFO_RESET, 0);
    if(rc != 0){
        PDEBUG("usb_gadget_ioctl(USB_IOCTL_FIFO_RESET) Error\n");
        return -1;
    }
    
    /* EP�ֹ� 1-15 ���˸��Ƥ��� */
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
                        
                        /* Ʊ��EP�ֹ椬�ɤ�Interface���������Ƥ��뤫��ǧ */
                        if(int_num == USB_GCORE_INT_NOT_DEF){
                            /* MaxPacketSize �� ��¸ */
                            max_mps = ift_element->us_max_psize;
                            
                            /* EP Address ����¸ */
                            ep_adr = ift_element->uc_ep_address;
                            
                            type = ift_element->uc_buffer_multiplex;
                            
                            /* Interface�ֹ����¸ */
                            int_num = if_desc->uc_if_number;
                            
                        }else if(int_num == if_desc->uc_if_number){
                            /* EP Address ����� */
                            if(ep_adr != ift_element->uc_ep_address){
                                rc = -1;
                                goto exit;
                            }
                            
                            if(type != ift_element->uc_buffer_multiplex){
                                rc = -1;
                                goto exit;
                            }
                            
                            /* �����MaxPacketSize����� */
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
            /* FIFO�����ѥѥ�᡼������ */
            fifo_param.bEpAdr = ep_adr;
            fifo_param.wSize = max_mps;
            fifo_param.bType = type;
            
            /* IOCTL_FIFO_MAP��Ƥ� */
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
        /* ���ｪλ�Ǥʤ����FIFO�򥯥ꥢ���� */
        PVERBOSE("usb_gadget_ioctl(USB_IOCTL_FIFO_RESET)\n");
        usb_gadget_ioctl(g_core->gadget, USB_IOCTL_FIFO_RESET, 0);
    }
    
    return rc;
}

#define USB_GADGET_CORE_UC_MAX_POWER_DEFAULT ( 0x32 )   // means 100mA.
/*����������������������������������������������������������������������*
 * start_interface
 *����������������������������������������������������������������������*
 * ����:    ������ä�Interface�ֹ��Alternate�ֹ椫��
 *          ep����������Config,Interface�ΰ��פ���FuncDrv��start()����
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          in_num    : Interface�ֹ�
 *          alt_num   : Alternate�ֹ�
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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
    
    /* ���Ϥ��� Config,Interface �˳�������InterfaceTable��������� */
    if_table = get_if_table_desc(g_core, g_core->set_config, in_num, alt_num);
    if(!if_table){
        PDEBUG("Error: Not found if_table\n");
        rc = -1;
        goto exit;
    }
    
    /* �ޥ�����EP�ꥹ���ΰ����ݤ��� */
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
    
    /* �ޥ�����EP�ꥹ�Ȥ���� */
    for(i = 0; i < if_table->uc_num_pep_list; i++){
        element = &if_table->pep_list[i];
        *(eps + i) = ep_ctrl_enable_ep(g_core, in_num, element);
        
        if(!*(eps + i)){
            rc = -1;
            goto exit;
        }
    }
    
    /* FuncDriver��start()���Τ��� */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->config == g_core->set_config &&
           tmp_func_drv->func_drv->interface == in_num             ){
            
            DEBUG_PRINT("start() to \"%s\"", 
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");

            /* FuncDrv��EP�ꥹ���ΰ����ݤ��� */
            if(if_table->uc_num_pep_list != 0){
                feps = kmalloc(sizeof(struct usb_ep*) * if_table->uc_num_pep_list, 
                               GFP_ATOMIC);
                DEBUG_NEW(feps);
                PVERBOSE("feps: 0x%lx\n", (unsigned long)feps);
                if(!feps){
                    PDEBUG("Error: kmalloc()\n");
                    
                    /* ���顼��ȯ�������顢interface����ߤ��ƽ�λ */
                    stop_interface(g_core, in_num);
                    rc = -1;
                    goto exit;
                }
            }
            
            /* FuncDrv��EP�ꥹ�Ȥ���� */
            for(i = 0; i < if_table->uc_num_pep_list; i++){
                *(feps + i) = ep_ctrl_create_ep(g_core, *(eps+i));
                if(!*(feps + i)){
                    PDEBUG("Error: kmalloc()\n");
                    
                    /* ���顼��ȯ�������顢���ޤǺ�ä�ep���� */
                    while(i > 0){
                        i--;
                        ep_ctrl_delete_ep(g_core, *(feps + i));
                    }
                    
                    /* interface����ߤ��ƽ�λ */
                    stop_interface(g_core, in_num);
                    rc = -1;
                    goto exit;
                }
            }
            
            /* ep_list����� */
            tmp_func_drv->ep_list.ep = feps;
            tmp_func_drv->ep_list.num_ep = if_table->uc_num_pep_list;;
            
            /* FuncDrv��start()���Τ����� */
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
        
        /* Special FuncDriver��start()���Τ��� */
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
    /* �ޥ�����EP�ꥹ���ΰ�������� */
    if(eps){
        PVERBOSE("kfree(eps) eps: 0x%lx\n", (unsigned long)eps);
        DEBUG_FREE(eps);
        kfree(eps);
    }
    
    return 0;
}

/*����������������������������������������������������������������������*
 * stop_interface
 *����������������������������������������������������������������������*
 * ����:    ������Interface�ֹ��ư���Ƥ���FuncDrv��stop()�����Τ�
 *          ep������disable�ˤ���
 * ����:    g_core     : GadgetCore�ؤΥݥ���
 *          in_num     : Interface�ֹ�
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int stop_interface(struct g_core_drv *g_core, u8 in_num)
{
    struct g_func_drv *tmp_func_drv;
    struct usb_ep **eps = NULL;
    u8 cfg_num, i;
    int rc;
    int stop_rsv = 0;
    
    cfg_num = g_core->set_config;
    
    PDEBUG("stop_interface() Cfg: %d In: %d\n", cfg_num, in_num);

    // DualMode�ˤ��MTPư��˾��֤����ܤ��Ƥ�����ϡ�
    // gcw�ˤ�ä�ư��Ƥ���funcDrv��SIC���ڤ��ؤ�äƤ��롣
    // ��äơ������Ǥ�SIC�Ѥ�config��õ����cfg_num���񤭤���
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

    /* ���ߤ�Config�ֹ桢�����Interface�ֹ��ư��Ƥ���FuncDrv��
       �Ф���stop()�����Τ��� */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->config == cfg_num   &&
           tmp_func_drv->func_drv->interface == in_num &&
           tmp_func_drv->started != 0           ){
            
            /* �����Ǿ��ΰ��פ���FuncDrv��stop()���Τ����� */
            PVERBOSE(" stop() to %s\n", 
                     tmp_func_drv->func_drv->function ?
                     tmp_func_drv->func_drv->function : "");
            tmp_func_drv->started = 0;
            stop_rsv = 1;
            DEBUG_PRINT("stop() to \"%s\"", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");
            tmp_func_drv->func_drv->stop(tmp_func_drv->func_drv);
            DEBUG_PRINT("stopped \"%s\"", tmp_func_drv->func_drv->function ? tmp_func_drv->func_drv->function : "");

            eps = tmp_func_drv->ep_list.ep;
            
            /* FuncDrv��EP�ꥹ�Ȥ��� */
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
    
    /* Interface������Ƥ�EP��disable�ˤ��� */
    rc = ep_ctrl_disable_ep(g_core, in_num);
    if(rc){
        PDEBUG("Error: ep_ctrl_disable_ep()\n");
    }
    
    return rc;
}

/*����������������������������������������������������������������������*
 * setup_set_config
 *����������������������������������������������������������������������*
 * ����:    ����������ɥꥯ������ SetConfig ���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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
    
    /* �ޤ���Descriptor Table ��˻����Config�ֹ��ConfigDescriptor�����뤫��ǧ */
    if(new_config != 0 && get_config_desc_from_config_num(g_core, new_config) == NULL){
        PDEBUG(" --->Fail\n");
        return -1; /* ���Ĥ���ʤ� */
    }
    
#ifdef IGNORE_SAME_SET_CONFIG
    if(new_config == old_config){
        /* ���ߤ�Config��Ʊ���ֹ��SetConfig���줿���ˤϲ��⤷�ʤ� */
        PDEBUG("SetConfig the same NowConfig: %d\n", old_config);
        
        /* EP0���̿���λ��˸ƤФ��ؿ���NULL�����ꤹ�� */
        g_core->ep0_comp_func = NULL;
        
        goto comp;
    }
#endif
    
    /* ���ߤ�Config��λ���� */
    rc = stop_config(g_core);
    if(rc != 0){
        return -1;
    }
    
    /* ������Config�򳫻Ϥ��� */
    rc = start_config(g_core, new_config);
    if(rc != 0){
        return -1;
    }
    
    /* EP0���̿���λ��˸ƤФ��ؿ������ꤹ�� */
    g_core->ep0_comp_func = setup_set_config_comp;
    
comp:
    /* EP0��Halt���֤������� */
    clear_halt_ep0(g_core);
    
    return 0;
}

static void
setup_set_config_comp(struct g_core_drv *g_core, struct usb_request *req)
{
    struct usb_kevent_arg_gadgetcore_set_config set_config;
    
    /* EP0�Ǥ��̿����������Ƥ����顢USB Svc��SetConfig���Τ�ȯ�Ԥ��� */
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

/*����������������������������������������������������������������������*
 * setup_get_config
 *����������������������������������������������������������������������*
 * ����:    ����������ɥꥯ������ GetConfig ���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 *          buf       : �����ѥХåե��ΰ�
 * ����:    0�ʾ� : ����(Host��ž�����륵����)
 *          0̤�� : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int setup_get_config(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf)
{
    int rc;
    u8 config;
    
    PDEBUG("GET_CONFIG\n");
    
    /* ���ߤ�Config�ͤ���� */
    config = g_core->set_config;
    
    rc = min(ctrl->wLength, (__le16)sizeof(config));
    
    memcpy(buf, &config, rc);
    
    return rc;
}

/*����������������������������������������������������������������������*
 * setup_set_interface
 *����������������������������������������������������������������������*
 * ����:    ����������ɥꥯ������ SetInterface ���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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
    
    /* Configured state ����ǧ */
    if(cfg_num == 0){
        return -EINVAL;
    }
    
    /* DescriptorTable��� ���ߤ�config�͡������Interface�͡�Alt�ͤ�
       if_table_desc�����뤫��ǧ */
    if(get_if_table_desc(g_core, cfg_num, in_num, new_alt_num) == NULL){
        PDEBUG("Not Found if_table desc Cfg: %d In: %d Alt: %d\n",
                  cfg_num, in_num, new_alt_num);
        return -EINVAL;
    }
    
#ifdef IGNORE_SAME_SET_INTERFACE
    if(old_alt_num == new_alt_num){
        /* ���ߤ�Alternate��Ʊ���ֹ��SetInterface���줿���ˤϲ��⤷�ʤ� */
        PDEBUG("SetInterface the same NowAlternate: %d\n", old_alt_num);
        
        /* EP0���̿���λ��˸ƤФ��ؿ���NULL�����ꤹ�� */
        g_core->ep0_comp_func = NULL;
        
        goto comp;
    }
#endif
    
    /* ���ߤ�Interface����� */
    rc = stop_interface(g_core, in_num);
    if(rc){
        return -1;
    }
    
    /* ������Alternate�ֹ��Interface�򳫻Ϥ��� */
    rc = start_interface(g_core, in_num, new_alt_num);
    if(rc){
        return -1;
    }
    
    /* Alternate�����ơ��֥���λ����Interface��Alternate�ֹ���ѹ� */
    if(set_alt_num(g_core, in_num, new_alt_num) != 0){
        PDEBUG("set_alt_num() Error\n");
        return -1;
    }
    
    /* EP0���̿���λ��˸ƤФ��ؿ������ꤹ�� */
    g_core->set_interface_info.interface_num = in_num;
    g_core->set_interface_info.alt_num = new_alt_num;
    g_core->ep0_comp_func = setup_set_interface_comp;
    
comp:
    /* EP0��Halt���֤������� */
    clear_halt_ep0(g_core);
    
    return 0;
}

static void
setup_set_interface_comp(struct g_core_drv *g_core, struct usb_request *req)
{
    struct usb_kevent_arg_gadgetcore_set_interface set_interface;
    
    /* EP0�Ǥ��̿����������Ƥ����顢USB Svc��SetConfig���Τ�ȯ�Ԥ��� */
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


/*����������������������������������������������������������������������*
 * setup_get_interface
 *����������������������������������������������������������������������*
 * ����:    ����������ɥꥯ������ GetInterface ���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 *          buf       : �����ѥХåե��ΰ�
 * ����:    0�ʾ� : ����(Host��ž�����륵����)
 *          0̤�� : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int setup_get_interface(struct g_core_drv *g_core,
                               const struct usb_ctrlrequest *ctrl,
                               void *buf)
{
    u8 in_num, alt_num;
    int rc;
    
    in_num = (u8)(ctrl->wIndex & 0x00ff);
    
    PDEBUG("GetInterface(Cfg: %d In: %d)\n", g_core->set_config, in_num);
    
    /* �����Interface��Alternate�ֹ����� */
    if(get_alt_num(g_core, in_num, &alt_num) != 0){
        PDEBUG(" --->Fail\n");
        return -EINVAL;
    }
    
    rc = min(ctrl->wLength, (__le16)sizeof(alt_num));
    
    memcpy(buf, &alt_num, rc);
    
    return rc;
}

/*����������������������������������������������������������������������*
 * setup_set_feature
 *����������������������������������������������������������������������*
 * ����:    ����������ɥꥯ������ SetFeature ���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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
        
        /* Config Descriptor����� */
        cfg_desc = get_current_config_desc(g_core);
        if(cfg_desc && (cfg_desc->uc_attributes & USB_CONFIG_ATT_WAKEUP)){
            /* �������֤Ȥ���RemoteWakeup�����Ĥ��줿���֤򵭲����Ƥ��� */
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
        
        /* �ƥ��ȥ⡼�ɤ򳫻Ϥ��� */
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
        
        /* �ƥ��ȥ⡼�ɤ����ä����Ȥ򵭲����� */
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
        /* wIndex, wLength�����������Ȥ��ǧ */
        if(ctrl->wIndex != 0 || ctrl->wLength != 0){
            rc = -EINVAL;
            break;
        }
#endif
        
        /* HNP���б����Ƥ��뤫��ǧ */
        if(!is_hnp_capable(g_core)){
            /* HNP���б����Ƥ��ʤ����STALL���� */
            rc = -EINVAL;
            break;
        }
        
        /* OTG Core��SetFeature���줿���Ȥ����� */
        usb_otgcore_gadget_set_feature(ctrl->wValue);
        
        rc = 0;
        break;
      
      default:
        rc = -EINVAL;
        break;
    }
    
    return rc;
}

/*����������������������������������������������������������������������*
 * set_halt_ep0
 *����������������������������������������������������������������������*
 * ����:    EP0���Ф��Ƥ�SetFeature���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int set_halt_ep0(struct g_core_drv *g_core)
{
    g_core->ep0_halt = 1;
    
    return 0;
}

/*����������������������������������������������������������������������*
 * clear_halt_ep0
 *����������������������������������������������������������������������*
 * ����:    EP0���Ф��Ƥ�ClearFeature���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int clear_halt_ep0(struct g_core_drv *g_core)
{
    g_core->ep0_halt = 0;
    
    return 0;
}

/*����������������������������������������������������������������������*
 * set_halt_ep
 *����������������������������������������������������������������������*
 * ����:    �̾�EP���Ф��Ƥ�SetFeature���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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
    
    /* Config�ֹ����� */
    cfg_num = g_core->set_config;

    //---
    // DualMode�ˤ��MTPư��˾��֤����ܤ��Ƥ�����ϡ�
    // gcw�ˤ�ä�ư��Ƥ���funcDrv��SIC���ڤ��ؤ�äƤ��롣
    // ��äơ������Ǥ�SIC�Ѥ�config��õ����cfg_num���񤭤���
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

    /* EP���ɥ쥹����Interface�ֹ����� */
    in_num = ep_ctrl_get_in_num(g_core, ep_adr);
    if(in_num == USB_GCORE_INT_NOT_DEF){
        PDEBUG("ep_adr not found\n");
        rc = -EINVAL;
        goto exit;
    }
    PDEBUG("in_num: %d\n", cfg_num);

    /* FuncDrv���椫�鳺������Interface��ư���Ƥ���FuncDrv��õ�� */
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
                
                /* EP�������� ���� EP���ɥ쥹������ ����ǧ */
                if(ep_info->use &&
                   ep_info->ep_adr == ep_adr){
                    
                    /* FunctionDriver��ep_st_halt() ��Ƥ� */
                    PDEBUG("ep_set_halt()\n");
                    rc = tmp_func_drv->func_drv->ep_set_halt(tmp_func_drv->func_drv,
                                                             ep);
                    if(rc == 0){
                        /* 0���֤�FuncDrv������Ф����ǽ�λ */
                        /* StatusStage�ؿʤ� */
                        rc = 0;
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALL���֤�FuncDrv������Ф����ǽ�λ */
                        /* EP0��ProtocolStall�ˤ��� */
                        rc = -EINVAL;
                        goto exit;
                    }
                }
            }
        }
    }
    
    /* �����򤹤�FuncDrv�����Ĥ���ʤ����ˤϡ�EP0��ProtocolStall�ˤ��� */
    rc = -EINVAL;
exit:
    return rc;
}


/*����������������������������������������������������������������������*
 * clear_halt_ep
 *����������������������������������������������������������������������*
 * ����:    �̾�EP���Ф��Ƥ�ClearFeature���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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
    
    /* Config�ֹ����� */
    cfg_num = g_core->set_config;

    //---
    // DualMode�ˤ��MTPư��˾��֤����ܤ��Ƥ�����ϡ�
    // gcw�ˤ�ä�ư��Ƥ���funcDrv��SIC���ڤ��ؤ�äƤ��롣
    // ��äơ������Ǥ�SIC�Ѥ�config��õ����cfg_num���񤭤���
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

    /* EP���ɥ쥹����Interface�ֹ����� */
    in_num = ep_ctrl_get_in_num(g_core, ep_adr);
    if(in_num == USB_GCORE_INT_NOT_DEF){
        PDEBUG("ep_adr not found\n");
        rc = -EINVAL;
        goto exit;
    }
    PDEBUG("in_num: %d\n", cfg_num);
    PDEBUG("ep_adr: %x\n", ep_adr);

    /* FuncDrv���椫�鳺������Interface��ư���Ƥ���FuncDrv��õ�� */
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
                
                /* EP�������� ���� EP���ɥ쥹������ ����ǧ */
                if(ep_info->use &&
                   ep_info->ep_adr == ep_adr){
                    
                    /* FunctionDriver��ep_st_halt() ��Ƥ� */
                    PDEBUG("ep_clear_halt()\n");
                    rc = tmp_func_drv->func_drv->ep_clear_halt(tmp_func_drv->func_drv,
                                                               ep);
                    if(rc == 0){
                        /* 0���֤�FuncDrv������Ф����ǽ�λ */
                        /* StatusStage�ؿʤ� */
                        rc = 0;
                        goto exit;
                    }else if(rc == USB_GADGETCORE_STALL){
                        /* STALL���֤�FuncDrv������Ф����ǽ�λ */
                        /* EP0��ProtocolStall�ˤ��� */
                        rc = -EINVAL;
                        goto exit;
                    }
                }
            }
        }
    }
    
    /* �����򤹤�FuncDrv�����Ĥ���ʤ����ˤϡ�EP0��ProtocolStall�ˤ��� */
    rc = -EINVAL;
exit:
    return rc;
}

/*����������������������������������������������������������������������*
 * get_status_ep0
 *����������������������������������������������������������������������*
 * ����:    EP0��GetStatus���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 * ����:    
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int get_status_ep0(struct g_core_drv *g_core,
                          const struct usb_ctrlrequest *ctrl,
                          void *buf)
{
    int rc;
    u16 status;
    
    /* EP0��Halt���֤�������� */
    rc = ep_ctrl_get_ep0_halt(g_core);
    status = 0;
    if(rc != 0){
        status = 1;
    }
    
    rc = min(ctrl->wLength, (__le16)sizeof(status));
    memcpy(buf, &status, rc);
    
    return rc;
}

/*����������������������������������������������������������������������*
 * get_status_ep
 *����������������������������������������������������������������������*
 * ����:    EPn��GetStatus���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ep_adr    : EP Address
 * ����:    
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int get_status_ep(struct g_core_drv *g_core,
                         const struct usb_ctrlrequest *ctrl,
                         void *buf)
{
    u16 status;
    int rc;
    
    /* Host�����EP���ɥ쥹��Stall���֤�������� */
    rc = ep_ctrl_get_ep_stall(g_core, ctrl->wIndex & 0x008f);
    if(rc > 0){
        /* Stall */
        status = 1;
    }else if(rc == 0){
        /* Not Stall */
        status = 0;
    }else{
        /* ��������EP���Ĥ���ʤ� -> EP0 Protocol Error */
        return -1;
    }
    
    rc = min(ctrl->wLength, (__le16)sizeof(status));
    memcpy(buf, &status, rc);
    
    return rc;
}

/*����������������������������������������������������������������������*
 * setup_clear_feature
 *����������������������������������������������������������������������*
 * ����:    ����������ɥꥯ������ ClearFeature ���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 * ����:     0 : ����
 *          !0 : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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
        
        /* Config Descriptor����� */
        cfg_desc = get_current_config_desc(g_core);
        if(cfg_desc && (cfg_desc->uc_attributes & USB_CONFIG_ATT_WAKEUP)){
            /* �������֤Ȥ���RemoteWakeup�����Ĥ��줿���֤򥯥ꥢ���Ƥ��� */
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

/*����������������������������������������������������������������������*
 * setup_get_status
 *����������������������������������������������������������������������*
 * ����:    ����������ɥꥯ������ GetStatus ���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 *          buf       : �����ѥХåե��ΰ�
 * ����:    0�ʾ� : ����(Host��ž�����륵����)
 *          0̤�� : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
static int setup_get_status(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf)
{
    usb_gadget_config_desc *cfg_desc;
    u8  i;
    int rc;
    u16 status;
    
    /* wValue ��0�����ǧ */
    if(ctrl->wValue != 0){
        return -EINVAL;
    }
    
    if(ctrl->bRequestType == (USB_DIR_IN        |
                              USB_TYPE_STANDARD |
                              USB_RECIP_DEVICE   )){
        PDEBUG("GetStatus(Device)\n");
        
        status = 0;
        
        /* SelfPower������ */
        if(g_core->set_config == 0){
            /* Address���֤ʤ��Default�ͤ���� */
            
            /* SpeedDesc��uc_default_attributes��Self-Powerd�ե饰���ǧ */
            if(g_core->desc_tbl->uc_default_attributes & USB_CONFIG_ATT_SELFPOWER){
                status |= (1 << USB_DEVICE_SELF_POWERED);
            }
        }else{
            /* Configured���֤ʤ��ConfigDesc���ͤ���� */
            
            cfg_desc = get_current_config_desc(g_core);
            if(cfg_desc == NULL){
                rc = -EINVAL;
                goto end;
            }
            
            /* ConfigDesc��uc_attributes��Self-Powerd�ե饰���ǧ */
            if(cfg_desc->uc_attributes & USB_CONFIG_ATT_SELFPOWER){
                status |= (1 << USB_DEVICE_SELF_POWERED);
            }
        }
        
        /* RemoteWakeup���ľ��֤����� */
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
            /* EP0 �ʳ� */
            PDEBUG("GetStatus(EPn)\n");
            rc = get_status_ep(g_core, ctrl, buf);
        }
        
    }else if(ctrl->bRequestType == (USB_DIR_IN         |
                                    USB_TYPE_STANDARD  |
                                    USB_RECIP_INTERFACE )){
        PDEBUG("GetStatus(Interface)\n");
        
        if(g_core->set_config == 0){
            /* Address���֤ʤ��Stall���� */
            rc = -EINVAL;
            goto end;
        }
        
        /* ���ߤ�ConfigDescriptor����� */
        cfg_desc = get_current_config_desc(g_core);
        if(cfg_desc == NULL){
            rc = -EINVAL;
            goto end;
            
        }
        
        /* �����Interface��¸�ߤ��뤫õ�� */
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

/*����������������������������������������������������������������������*
 * setup_get_ms_descriptor
 *����������������������������������������������������������������������*
 * ����:    �ꥯ������ GetMSDescriptor ���������
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 *          buf       : �����ѥХåե��ΰ�
 * ����:    0�ʾ� : ����(Host��ž�����륵����)
 *          0̤�� : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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

/*����������������������������������������������������������������������*
 * setup_req
 *����������������������������������������������������������������������*
 * ����:    SETUP��������Ȥ��˸ƤФ��
 * ����:    g_core    : GadgetCore�ؤΥݥ���
 *          ctrl      : SETUP�ѥ��åȥǡ���
 * ����:    0�ʾ� : ����(Host��ž�����륵����)
 *          0̤�� : ����
 * 
 * <EOS>                                *
 *����������������������������������������������������������������������*/
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
          
            /* EP0��Halt��ʤ��STALL���� */
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
                    /* FS����⡼�ɤʤ��STALL���� */
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
                    /* FS����⡼�ɤʤ��STALL���� */
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
            /* EP0��Halt��ʤ��STALL���� */
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
            /* EP0��Halt��ʤ��STALL���� */
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

