/*
 * usb_gcore_wrapper.c
 *
 * Copyright 2011,2013 Sony Corporation
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

#ifdef DBG_PREFIX
# undef  DBG_PREFIX
# define DBG_PREFIX "GCORE_WRAPPER"
#else
# define DBG_PREFIX "GCORE_WRAPPER"
#endif

#include "usb_gcore_wrapper_pvt.h"
#include "usb_gcore_wrapper_cfg.h"

//-------
// for DEBUG
#define STATE_COLOR_ON  "\x1b[1;32m"
#define STATE_COLOR_OFF "\x1b[0m"

char* get_gcw_state_str( GCW_STATE state ){
    switch( state ){
      case GCW_STATE_NONE:
        return STATE_COLOR_ON "GCW_STATE_NONE" STATE_COLOR_OFF;
      case GCW_STATE_INITIALIZED:
        return STATE_COLOR_ON "GCW_STATE_INITIALIZED" STATE_COLOR_OFF;
      case GCW_STATE_SINGLE:
        return STATE_COLOR_ON "GCW_STATE_SINGLE" STATE_COLOR_OFF;
      case GCW_STATE_DUAL_IDLE:
        return STATE_COLOR_ON "GCW_STATE_DUAL_IDLE" STATE_COLOR_OFF;
      case GCW_STATE_DUAL_STARTED:
        return STATE_COLOR_ON "GCW_STATE_DUAL_STARTED" STATE_COLOR_OFF;
      case GCW_STATE_DUAL_CONVERTING:
        return STATE_COLOR_ON "GCW_STATE_DUAL_CONVERTING" STATE_COLOR_OFF;
      case GCW_STATE_DUAL_FIXED:
        return STATE_COLOR_ON "GCW_STATE_DUAL_FIXED" STATE_COLOR_OFF;
      case GCW_STATE_DUAL_FIXED_SIC:
        return STATE_COLOR_ON "GCW_STATE_DUAL_FIXED_SIC" STATE_COLOR_OFF;
      case GCW_STATE_LAST:
        return STATE_COLOR_ON "GCW_STATE_LAST" STATE_COLOR_OFF;
      default:
        return STATE_COLOR_ON "UNKNOWN GCW_STATE!!" STATE_COLOR_OFF;
    }
}
#define GET_STATE_STR() get_gcw_state_str(get_gcw_state())
//-------

static struct gcw_info g_gcw_info;

static GCW_LOCK_INFO* p_gcw_lock_info = NULL;
#define LOCK(x)   spin_lock_irqsave(&p_gcw_lock_info->lock,(x))
#define UNLOCK(x) spin_unlock_irqrestore(&p_gcw_lock_info->lock,(x))

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

GCW_BOOL change_gcw_state( GCW_STATE new_state ){
    unsigned long lock_flags;

    LOCK(lock_flags);
    DEBUG_INFO("!! current_state = %s", get_gcw_state_str( g_gcw_info.state ) );
    DEBUG_INFO("!! ->> and new_state = %s", get_gcw_state_str( new_state ) );
    g_gcw_info.state = new_state;
    UNLOCK(lock_flags);

    return GCW_TRUE;
}

GCW_STATE get_gcw_state( void ){
    GCW_STATE state;
    unsigned long lock_flags;

    LOCK(lock_flags);
    state = g_gcw_info.state;
    UNLOCK(lock_flags);
    return state;
}

extern void gcore_complete(struct usb_ep*, struct usb_request*);

int does_match_epadr( struct g_core_ep *gep, u8 ep_adr ){
    int ret=GCW_FALSE;
    u8 _ep_adr;

    if( gep == NULL ){
        DEBUG_ERR("gep is NULL");
        goto exit;
    }

    _ep_adr = GET_EP_ADR( gep );

    if( _ep_adr == ep_adr ){
        ret = GCW_TRUE;
    }
    else{
        ret = GCW_FALSE;
    }

  exit:
    return ret;
}

int is_BULKOUT_EP( struct g_core_ep *gep ){
    DEBUG_INFO("%s", __FUNCTION__);
    return does_match_epadr( gep, EPADR_DUALMODE_BULKOUT_EPADR );
}

int is_BULKOUT_MTP( u8* buff, int size ){
    int  ret=GCW_FALSE;
    u32* p_cmd_hdr;
    u16  cmd_id;

    DEBUG_INFO("%s", __FUNCTION__);
    if( buff == NULL ){
        DEBUG_ERR("buff is NULL");
        ret = GCW_FALSE;
        goto exit;
    }

    if( size < 0 ){
        DEBUG_ERR("size is < 0");
        ret = GCW_FALSE;
        goto exit;
    }

    p_cmd_hdr = (u32 *)buff;
    cmd_id    = *(u16 *)(p_cmd_hdr + 1);

    if((*p_cmd_hdr <= MTP_COMMAND_HEADER) && (cmd_id == MTP_COMMAND_ID)){
        ret = GCW_TRUE;
    }
    else{
        ret = GCW_FALSE;
    }

  exit:
    return ret;
}

/*-----------------------------------------------------------------------------
 * for usb_gcore_main.c
 *---------------------------------------------------------------------------*/
static struct usb_gadget_driver g_gcw_driver;   //For GCoreWrapper
static struct usb_gadget_driver g_gcore_driver; //For GCoreDriver
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
    static int (*gp_bind)(struct usb_gadget *);
#endif

int
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
usb_gcw_probe_driver(struct usb_gadget_driver* gcore_driver, int (*bind)(struct usb_gadget *)){
#else
usb_gcw_register_driver(struct usb_gadget_driver* gcore_driver){
#endif

    DEBUG_INFO("%s", __FUNCTION__);
    if( gcore_driver == NULL ){
        DEBUG_ERR("gcore_driver is NULL");
        return -1;
    }

    // initialize the lock info
    p_gcw_lock_info = kmalloc( sizeof( GCW_LOCK_INFO ), GFP_ATOMIC );
    memset( p_gcw_lock_info, 0, sizeof( GCW_LOCK_INFO ) );
    spin_lock_init( &p_gcw_lock_info->lock );

    // Store the passed driver callback data
    memcpy( &g_gcw_driver,   gcore_driver, sizeof(g_gcw_driver) );

    // Overwrite the callback to hook
    memcpy( &g_gcore_driver, gcore_driver, sizeof(g_gcore_driver) );
    g_gcore_driver.unbind     = gcw_unbind;
    g_gcore_driver.setup      = gcw_setup;
    g_gcore_driver.disconnect = gcw_disconnect;
    g_gcore_driver.suspend    = gcw_suspend;
    g_gcore_driver.resume     = gcw_resume;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
    gp_bind = bind;
#else
    g_gcore_driver.bind       = gcw_bind;
#endif
    
    // no check for the state here
    change_gcw_state( GCW_STATE_INITIALIZED );
    
    // register the hooked driver callback data
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
    return usb_gadget_probe_driver(&g_gcore_driver, gcw_bind);
#else
    return usb_gadget_register_driver(&g_gcore_driver);
#endif
}
int
usb_gcw_unregister_driver(struct usb_gadget_driver* gcore_driver){
    int res;
    DEBUG_INFO("%s", __FUNCTION__);
    if( gcore_driver == NULL ){
        DEBUG_ERR("gcore_driver is NULL");
        return -1;
    }

    // unregister the hooked driver callback data
    res = usb_gadget_unregister_driver(&g_gcore_driver);

    // Delete the passed driver callback data
    memset( &g_gcw_driver,   0x00, sizeof(g_gcw_driver) );
    memset( &g_gcore_driver, 0x00, sizeof(g_gcore_driver) );

    // check and change state
    if( get_gcw_state() == GCW_STATE_INITIALIZED ){
        DEBUG_ERR("Err in check_state!! now %s...", get_gcw_state_str(get_gcw_state()) );
        DEBUG_ERR("But Force to GCW_STATE_NONE");
    }
    change_gcw_state( GCW_STATE_NONE );

    // finalize the lock info
    if ( p_gcw_lock_info ){
        kfree( p_gcw_lock_info );
        p_gcw_lock_info = NULL;
    }

    return res;
}
int usb_gcw_start( GCW_BOOL enable_flag ){
    DEBUG_INFO("%s", __FUNCTION__);

    // 状態チェック
    if( get_gcw_state() != GCW_STATE_INITIALIZED ){
        DEBUG_ERR("Err in check_state!! now %s...", get_gcw_state_str(get_gcw_state()) );
        return -1;
    }

    // 処理実行
    if( enable_flag == GCW_TRUE ){
        // 状態変化
        change_gcw_state( GCW_STATE_DUAL_IDLE );
    }
    else{
        // 状態変化
        change_gcw_state( GCW_STATE_SINGLE );
    }
    return 0;
}
int usb_gcw_stop(void){
    // 状態チェック
    // 無し
    DEBUG_PRINT("Called usb_gcw_stop() in  %s...", get_gcw_state_str(get_gcw_state()) );

    // 処理実行
    change_gcw_state( GCW_STATE_INITIALIZED );

    return 0;
}

int
gcw_bind(struct usb_gadget *gadget)
{
    DEBUG_INFO("%s", __FUNCTION__);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
    if( gp_bind == NULL ){
#else
    if( g_gcw_driver.bind == NULL ){
#endif
        DEBUG_ERR("callback not set!");
        return -1;
    }
    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
    return gp_bind( gadget );
#else
    return g_gcw_driver.bind( gadget );
#endif
}

void
gcw_unbind(struct usb_gadget *gadget)
{
    DEBUG_INFO("%s", __FUNCTION__);
    if( g_gcw_driver.unbind == NULL ){
        DEBUG_ERR("callback not set!");
        return;
    }

    g_gcw_driver.unbind( gadget );
    return;
}

int
gcw_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
    DEBUG_INFO("%s", __FUNCTION__);
    if( g_gcw_driver.setup == NULL ){
        DEBUG_ERR("callback not set!");
        return -1;
    }

    return g_gcw_driver.setup( gadget, ctrl );

}
void gcw_disconnect(struct usb_gadget *gadget)
{
    DEBUG_INFO("%s", __FUNCTION__);
    if( g_gcw_driver.disconnect == NULL ){
        DEBUG_ERR("callback not set!");
        return;
    }

    g_gcw_driver.disconnect( gadget );
    return;
}

void gcw_suspend(struct usb_gadget *gadget)
{
    DEBUG_INFO("%s", __FUNCTION__);
    if( g_gcw_driver.suspend == NULL ){
        DEBUG_ERR("callback not set!");
        return;
    }

    g_gcw_driver.suspend( gadget );
    return;
}

void gcw_resume(struct usb_gadget *gadget)
{
    DEBUG_INFO("%s", __FUNCTION__);
    if( g_gcw_driver.resume == NULL ){
        DEBUG_ERR("callback not set!");
        return;
    }

    g_gcw_driver.resume( gadget );
    return;
}



/*-----------------------------------------------------------------------------
 * for usb_gcore_setup.c
 *---------------------------------------------------------------------------*/
int usb_gcw_setup_setconfig( u8 new_config ){
    GCW_STATE current_state;
    GCW_STATE next_state;

    DEBUG_INFO("in %s", __FUNCTION__);

    // =======
    // Check the gcw_state
    // =======
    current_state = get_gcw_state();
    if( (current_state == GCW_STATE_DUAL_IDLE)       ||
        (current_state == GCW_STATE_DUAL_STARTED)    ||
        (current_state == GCW_STATE_DUAL_CONVERTING) ||
        (current_state == GCW_STATE_DUAL_FIXED)      ||
        (current_state == GCW_STATE_DUAL_FIXED_SIC)  ){
        
    }
    else if( (current_state == GCW_STATE_SINGLE) ){
        DEBUG_INFO("STATE[%s] do nothing in %s", get_gcw_state_str(current_state), __FUNCTION__ );
        return 0;
    }
    else{
        DEBUG_ERR("STATE_ERR[%s] in %s", get_gcw_state_str(current_state), __FUNCTION__ );
        return -1;
    }

    // =======
    // Change State!
    // =======
    switch( new_config ){
      case 0:
        DEBUG_PRINT("changed to NO config=%d", new_config);
        next_state = GCW_STATE_DUAL_IDLE;
        break;
      case AUTOMODE_CONFIGNUM:
        DEBUG_PRINT("changed to AUTO config=%d", new_config);
        next_state = GCW_STATE_DUAL_STARTED;
        break;
      default:
        DEBUG_PRINT("changed to OTHER config=%d", new_config);
        next_state = GCW_STATE_DUAL_FIXED;
        break;
    }

    if( current_state == next_state ){
        DEBUG_PRINT("Current State is already %s", get_gcw_state_str(next_state) );
        return 0;
    }

    change_gcw_state( next_state );
    return 0;
}

int usb_gcw_setup_setinterface( u8 new_config ){
    DEBUG_INFO("in %s", __FUNCTION__);
    return usb_gcw_setup_setconfig( new_config );
}

/*-----------------------------------------------------------------------------
 * for usb_gcore_ep.c
 *---------------------------------------------------------------------------*/
static struct usb_ep_ops g_gcore_ep_ops;

int usb_gcw_register_ep_ops(struct usb_ep_ops* ep_ops){
    DEBUG_INFO("%s", __FUNCTION__);
    if( ep_ops == NULL ){
        DEBUG_ERR("ep_ops is NULL");
        return -1;
    }

    memcpy( &g_gcore_ep_ops, ep_ops, sizeof(g_gcore_ep_ops) );
    return 0;
}
int usb_gcw_unregister_ep_ops(struct usb_ep_ops* ep_ops){
    DEBUG_INFO("%s", __FUNCTION__);

    memset( &g_gcore_ep_ops, 0x00, sizeof(g_gcore_ep_ops) );
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
struct usb_request *
gcw_alloc_request (struct usb_ep *ep, gfp_t gfp_flags)
#else
struct usb_request *
gcw_alloc_request (struct usb_ep *ep, int gfp_flags)
#endif
{
    DEBUG_INFO("%s", __FUNCTION__);
    return g_gcore_ep_ops.alloc_request( ep, gfp_flags );
}

void
gcw_free_request (struct usb_ep *ep, struct usb_request *req)
{
    DEBUG_INFO("%s", __FUNCTION__);
    g_gcore_ep_ops.free_request( ep, req );
    return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
void *
gcw_alloc_buffer (
    struct usb_ep           *ep,
    unsigned                bytes,
    dma_addr_t              *dma,
    gfp_t                   gfp_flags
    )
#else
void *
gcw_alloc_buffer (
    struct usb_ep           *ep,
    unsigned                bytes,
    dma_addr_t              *dma,
    int                     gfp_flags
    )
#endif
{
    DEBUG_INFO("%s", __FUNCTION__);
    g_gcore_ep_ops.alloc_buffer( ep, bytes, dma, gfp_flags );
    return;
}

void
gcw_free_buffer (
    struct usb_ep *ep,
    void *buf,
    dma_addr_t dma,
    unsigned bytes
    )
{
    DEBUG_INFO("%s", __FUNCTION__);
    g_gcore_ep_ops.free_buffer( ep, buf, dma, bytes );
    return;
}

#endif

void
gcw_complete(struct usb_ep *_ep, struct usb_request *_req)
{
    struct g_core_ep *gep;
    struct g_core_request *greq;
    void   (*complete)(struct usb_ep *ep, struct usb_request *req);
    void   *context;
    unsigned long lock_flags;
    struct usb_gadget_ep_list ep_list;

    DEBUG_INFO("%s", __FUNCTION__);

    /*
    DEBUG_INFO("_ep : 0x%lx", (unsigned long)_ep);
    DEBUG_INFO("_req : 0x%lx", (unsigned long)_req);
      */

    greq = (struct g_core_request*)_req->context;
    gep = greq->gep;

    /*
    DEBUG_INFO("gep : 0x%lx", (unsigned long)gep);
    DEBUG_INFO("greq : 0x%lx", (unsigned long)greq);
    DEBUG_INFO("gep->_ep : 0x%lx", (unsigned long)gep->_ep);
    DEBUG_INFO("greq->_req : 0x%lx", (unsigned long)greq->_req);
      */

    if(greq->req.complete){
        complete = greq->req.complete;
        context = greq->req.context;
        memcpy(&greq->req, _req, sizeof(greq->req));

        greq->req.complete = complete;
        greq->req.context = context;

        // Check state.
        if( get_gcw_state() == GCW_STATE_DUAL_STARTED ){
            DEBUG_INFO("The Correct State. [%s]", GET_STATE_STR() );

            // Check if this complete is for BULKOUT_EP
            if( GCW_TRUE == is_BULKOUT_EP( gep ) ){
                DEBUG_PRINT("The Target EP_ADR is BULKOUT!!");

                // Check if this BULKOUT_PACKET is MTP Packet
                if( GCW_TRUE == is_BULKOUT_MTP( (u8*)_req->buf, _req->actual ) ){
                    DEBUG_PRINT("\x1b[7;35m### This is MTP BULKOUT packet !!\x1b[0m");

                    // Now Copy this buffer to relay_buff
                    LOCK(lock_flags);
                    memset( g_gcw_info.buff, 0x00, sizeof(g_gcw_info.buff) );
                    g_gcw_info.buff_len = _req->actual;
                    DEBUG_PRINT("Copying the buff data...");
                    memcpy( g_gcw_info.buff, _req->buf, _req->actual );
                    DEBUG_PRINT("### _req->buf DATA !!");
                    UNLOCK(lock_flags);

                    // Dump this packet for debug here
                    DEBUG_PRINT("gcore_complete");
                    DEBUG_DUMP( (char*)_req->buf, _req->actual );

                    // change the state to CONVERTING at last.
                    change_gcw_state( GCW_STATE_DUAL_CONVERTING );

                    // Stop the MSC Class/SubClass/Protocol gadgetDrvs.
                    DEBUG_PRINT("Now Stopping MSC gadgetDrvs");
                    ep_list = stopFuncDrv_fromIfClasses( MSC_IF_CLASS, MSC_IF_SUBCLASS, MSC_IF_PROTOCOL);

                    // Start the SIC Class/SubClass/Protocol gadgetDrvs.
                    DEBUG_PRINT("Now Starting SIC gadgetDrvs");
                    startFuncDrv_fromIfClasses( SIC_IF_CLASS, SIC_IF_SUBCLASS, SIC_IF_PROTOCOL, ep_list, USB_GCORE_STARTEXT_BYDUAL);
                    DEBUG_PRINT("Started!!");

                    // when changing, this function returns without Calling complete() callback.
                    return; 
                }
                else{
                    DEBUG_PRINT("\x1b[7;31m### I dont think this packet is for mtp... !!\x1b[0m");

                    // Dump this packet for debug here
                    DEBUG_PRINT("\x1b[1;31m gcore_complete vv");
                    DEBUG_DUMP( (char*)_req->buf, _req->actual );
                    DEBUG_PRINT("\x1b[0m");

                    // change the state to FIXED here.
                    change_gcw_state( GCW_STATE_DUAL_FIXED );
                }
            }
            else{
                DEBUG_ERR("The Target EP_ADR want BULKOUT!!");
            }
        }
        else if( get_gcw_state() == GCW_STATE_SINGLE ){
            DEBUG_INFO("The State is SINGLE!!");
        }
        else if( get_gcw_state() == GCW_STATE_DUAL_CONVERTING ){
            DEBUG_INFO("The State is CONVERTING!!");
            return;
        }
        else if( get_gcw_state() == GCW_STATE_DUAL_FIXED ){
            DEBUG_INFO("The State is FIXED!!");
        }
        else if( get_gcw_state() == GCW_STATE_DUAL_FIXED_SIC ){
            DEBUG_INFO("The State is FIXED_SIC!!");
        }
        else{
            DEBUG_ERR("The State Err. [%s]", GET_STATE_STR() );
        }

        complete(&gep->ep, &greq->req);
    }

    return;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
int
gcw_queue (struct usb_ep *ep, struct usb_request *req, gfp_t gfp_flags)
#else
int
gcw_queue (struct usb_ep *ep, struct usb_request *req, int gfp_flags)
#endif
{
    struct g_core_ep *gep;
    struct g_core_request *greq;
    unsigned long lock_flags;

    DEBUG_INFO("%s", __FUNCTION__);

    gep = container_of(ep, struct g_core_ep, ep);
    greq = container_of(req, struct g_core_request, req);

#if 1
    if( GCW_TRUE == is_BULKOUT_EP( gep ) ){
        DEBUG_INFO("this is bulkout_ep!!!");
        if( get_gcw_state() == GCW_STATE_DUAL_CONVERTING ){
            DEBUG_PRINT("Now in CONVERTING. Will relay the mtp packet!!!");

            // Relays the mtp packet data for the first ep_queue() from funcDrv.
            LOCK(lock_flags);
            DEBUG_INFO("#### reqlen=%d, relaylen=%d", req->length, g_gcw_info.buff_len);
            if( g_gcw_info.buff_len < req->length ){
                req->actual = g_gcw_info.buff_len;
                memcpy( req->buf, g_gcw_info.buff, req->actual );
                memcpy( req->context, g_gcw_info.buff, req->actual );
            }
            else{
                DEBUG_ERR("relayed buff is longer than request buff len!!");
                DEBUG_ERR("reqlen=%d, relaylen=%d", req->length, g_gcw_info.buff_len);
            }
            UNLOCK(lock_flags);

            // Dump this packet for debug here
            DEBUG_INFO("\x1b[1;31m gcw_queue vv \x1b[0m");
            DEBUG_DUMP( (char*)req->buf, req->actual );
            DEBUG_INFO("relayed data!! : and Call complete!!!");

            // Call the complete callback in this
            if( req->complete != NULL ){
                req->complete(ep, req);
            }
            else{
                DEBUG_ERR("req->complete is NULL");
            }
            // Change the state to FIXED. Will never relay a packet while in this state.
            change_gcw_state( GCW_STATE_DUAL_FIXED_SIC );

            // when data was relayed returns without calling ep_queue().
            return 0;
        }
        else{
            DEBUG_INFO("Not in CONVERTING. now in %s", GET_STATE_STR() );
            DEBUG_INFO("Will not relay the mtp packet, normal ep_queue.");
        }
    }
    else{
        DEBUG_INFO("this is not bulkout_ep!!!");
    }
#endif

    DEBUG_INFO("gep : 0x%lx", (unsigned long)gep);
    DEBUG_INFO("greq : 0x%lx", (unsigned long)greq);
    DEBUG_INFO("gep->_ep : 0x%lx", (unsigned long)gep->_ep);
    DEBUG_INFO("greq->_req : 0x%lx", (unsigned long)greq->_req);

    memcpy(greq->_req, req, sizeof(*greq->_req));

    greq->_req->complete = gcw_complete;
    greq->_req->context = greq;

    greq->gep = gep;

    return usb_ep_queue(gep->_ep, greq->_req, gfp_flags);
}

int
gcw_dequeue (struct usb_ep *ep, struct usb_request *req)
{
    DEBUG_INFO("%s", __FUNCTION__);
    return g_gcore_ep_ops.dequeue( ep, req );
}

void
gcw_fifo_flush(struct usb_ep *ep)
{
    DEBUG_INFO("%s", __FUNCTION__);
    g_gcore_ep_ops.fifo_flush( ep );
    return;
}

int
gcw_fifo_status(struct usb_ep *ep)
{
    DEBUG_INFO("%s", __FUNCTION__);
    return g_gcore_ep_ops.fifo_status( ep );
}

int
gcw_set_halt(struct usb_ep *ep, int value)
{
    DEBUG_INFO("%s", __FUNCTION__);
    return g_gcore_ep_ops.set_halt( ep, value );
}

