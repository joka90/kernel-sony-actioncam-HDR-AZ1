/*
 * usb_otgcore.c
 * 
 * Copyright 2005,2006,2008,2009,2011,2013 Sony Corporation
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
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/interrupt.h> // For irqreturn_t.

#include <linux/udif/mutex.h>

#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif

#include <linux/udif/cdev.h>
#include <mach/udif/devno.h>

#if defined(CONFIG_ARCH_CXD4132BASED)
#   include <linux/usb/scd/usb_otg_control.h>
#   if defined(CONFIG_UPS) || defined(CONFIG_UPS_MODULE)
#       include <linux/usb/scd/ups.h>
#   endif
#elif defined(CONFIG_ARCH_CXD4120)
#   include <linux/usb/mcd/usb_otg_control.h>
#elif defined(CONFIG_ARCH_CXD90014BASED)
#   include <linux/usb/f_usb/usb_otg_control.h>
#else
#   error CONFIG_ARCH error
#endif

#include <linux/udif/io.h>

#include <linux/usb/gcore/usb_event.h>
#include <linux/usb/gcore/usb_otgcore.h>
#include <linux/gpio/gpio.h>

#include "usb_otgcore_cfg.h"
#include "usb_otgcore_pvt.h"
#include "usb_otgcore_ups.h"

/*-----------------------------------------------------------------------------
 * Module infomation
 *---------------------------------------------------------------------------*/
MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION(USBOTGCORE_NAME
                   " driver ver " USBOTGCORE_VERSION);
MODULE_LICENSE("GPL");

#define MYDRIVER_NAME    USBOTGCORE_NAME

// #define USB_OTGCORE_VBUS_ID_IRQ_BY_ME


/*-----------------------------------------------------------------------------
 * Function prototype declaration
 *---------------------------------------------------------------------------*/
int __init usb_otgcore_module_init(void);
static void __exit usb_otgcore_module_exit(void);
static int proc_read(char *, char **, off_t, int, int *, void *);

static int __init otg_core_alloc(void);
static void otg_core_free(struct otg_core_drv *otg_core);
static int otg_core_bind(struct usb_otg_control *otg_control);
static int otg_core_unbind(struct usb_otg_control *otg_control);
static int otg_core_notify(union usb_otg_event *otg_event);

static int event_cid_exec(struct usb_otg_event_cid *cid);
static int event_vbus_exec(struct usb_otg_event_vbus *vbus);
static int event_vbus_error_exec(void);
static int event_receive_srp_exec(void);
static int event_rchost_end_exec(unsigned int);
static int event_rcgadget_end_exec(unsigned int);
static int event_pullup_exec(struct usb_otg_event_con *con);
static int do_get_line_state( struct usb_otgcore_line_state *ls );

static int transition_to_idle(void);
static int transition_to_stop(void);

static irqreturn_t usb_otgcore_vbus_IrqHandler( int irq, void* p_dev_id );
static irqreturn_t usb_otgcore_vbus_stda_IrqHandler( int irq, void* p_dev_id );
#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
static irqreturn_t usb_otgcore_id_IrqHandler( int irq, void* p_dev_id );
#endif
static void usb_otgcore_notify_vbusoff_cidb( void );
static void usb_otgcore_notify_current_vbus_state( void );
static void usb_otgcore_notify_current_id_state( void );
static void usb_otgcore_request_vbusid_irq( void );
static void usb_otgcore_free_vbusid_irq( void );

/* for PHY suspend */
static void set_phy_suspend( void );
/* for PHY Param Setting */
static void set_phy_params( void );

/*-----------------------------------------------------------------------------
 * Variable declaration
 *---------------------------------------------------------------------------*/
static struct otg_core_drv *the_otg_core;

static struct usb_otg_core otg_core_driver =
{
    .bind   = otg_core_bind,
    .unbind = otg_core_unbind,
    .notify = otg_core_notify,
};

const static struct usb_otgcore_ops pr_otg_core_ops = 
{
};

static struct usb_otgcore pr_otg_core = 
{
    .ops = &pr_otg_core_ops,
};

static struct usb_otg_control_port_info the_port_info;
static struct ups_port_descriptor the_port_desc;
static struct usb_otgcore_phy_param st_phy_param;

static int vbus_irq_no = -1;
static int vbus_stda_irq_no = -1;

/*=============================================================================
 *
 * Main function body
 *
 *===========================================================================*/
int usb_otgcore_gadget_suspend(void)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int res;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    if(test_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags)){
        /* enable_rchost()済みならばRollChangeの開始と判断 */
        res = USB_OTGCORE_RES_START_RCHOST;
    }else{
        /* enable_rchost()済みでなければ通常のsuspendと判断 */
        res = USB_OTGCORE_RES_SUSPEND;
    }
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    if(res == USB_OTGCORE_RES_START_RCHOST){
        PVERBOSE("result: USB_OTGCORE_RES_START_RCHOST\n");
    }else{
        PVERBOSE("result: USB_OTGCORE_RES_SUSPEND\n");
    }
    
    return res;
}

int usb_otgcore_register_driver(struct usb_otg_driver *drv)
{
    int err = 0;
    struct otg_core_drv *otg_core = the_otg_core;
    struct m_otg_drv *new_otg_drv, *tmp_otg_drv;
    unsigned long flags;
    
    PDEBUG("%s call\n", __func__);
    
    new_otg_drv = (struct m_otg_drv*)kmalloc(sizeof(struct m_otg_drv), GFP_ATOMIC);
    if(!new_otg_drv){
        PERR("kmalloc(): Fail\n");
        err = -ENOMEM;
        goto SUB_RET;
    }
    
    PDEBUG("usb_otg_driver: 0x%08lx\n", (unsigned long)drv);
    new_otg_drv->otg_drv = drv;
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* FunctionDriverがリストに無いことを確認する */
    list_for_each_entry(tmp_otg_drv, &otg_core->drv_list, list){
        if(tmp_otg_drv->otg_drv == new_otg_drv->otg_drv){
            err = -EINVAL;
        }
    }
    
    /* OTG Driverをリストに追加 */
    if(err == 0){
        list_add_tail(&new_otg_drv->list, &otg_core->drv_list);
    }
    
    /* OTG Driverへbind発行 */
    if(drv->bind){
        drv->bind(drv, &pr_otg_core);
    }
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    if(err != 0){
        PDEBUG("%s error!!\n", __func__);
        kfree(new_otg_drv);
        goto SUB_RET;
    }
    
    PDEBUG("========================================================\n");
    PDEBUG("OTGDriver(%s) Added\n", drv->function ? drv->function : "");
    PDEBUG(" : 0x%08lx\n", (unsigned long)drv);
    PDEBUG(" bind():               0x%08lx\n", (unsigned long)drv->bind);
    PDEBUG(" unbind():             0x%08lx\n", (unsigned long)drv->unbind);
    PDEBUG(" notify():             0x%08lx\n", (unsigned long)drv->notify);
    PDEBUG(" query_over_current(): 0x%08lx\n", 
        (unsigned long)drv->query_over_current);
    PDEBUG("========================================================\n");
    
SUB_RET:
    return err;
}

int usb_otgcore_unregister_driver(struct usb_otg_driver *drv)
{
    int err = 0;
    struct otg_core_drv *otg_core = the_otg_core;
    struct m_otg_drv *tmp_otg_drv, *n;
    unsigned long flags;
    
    PDEBUG("%s call\n", __func__);
    
    err = -EFAULT;
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* リストから指定のFunctionDriverを探す */
    list_for_each_entry_safe(tmp_otg_drv, n, &otg_core->drv_list, list){
        if(tmp_otg_drv->otg_drv == drv){
            /* 指定のFunctionDriverが見つかったらリストから削除 */
            PDEBUG("del otg_drv: 0x%08lx\n", (unsigned long)tmp_otg_drv->otg_drv);
            PDEBUG("tmp_otg_drv: 0x%08lx\n", (unsigned long)tmp_otg_drv);
            list_del(&tmp_otg_drv->list);
            kfree(tmp_otg_drv);
            
            err = 0;
        }
    }
    
    /* OTG Driverへunbind発行 */
    if(drv->unbind){
        drv->unbind(drv, &pr_otg_core);
    }
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    return err;
}

unsigned char usb_otgcore_get_hs_disable(void)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    unsigned char hs_disable;
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    hs_disable = otg_core->hs_disable;
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    return hs_disable;
}

int usb_otgcore_gadget_set_feature(__le16 feature_selector)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int res = 0;
    
    PDEBUG("%s call\n", __func__);
    PVERBOSE(" feature_selector: %04x\n", feature_selector);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現状では、b_hnp_enableのみを処理する */
    if(feature_selector != USB_DEVICE_B_HNP_ENABLE){
        goto exit;
    }
    
    /* Hostによって HNP が許可されたことを記録 */
    set_bit(USB_OTGCORE_ATOMIC_HNP_ENABLE_HOST, &otg_core->bitflags);
    
    /* Userによって HNP が許可されている             &&
       usb_otg_control_enable_rchost() 発行済みでない    */
    if(test_bit(USB_OTGCORE_ATOMIC_HNP_ENABLE_USER, &otg_core->bitflags) &&
       test_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags) == 0){
        
        set_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags);
        
        /* rchostを許可する */
        PDEBUG("usb_otg_control_enable_rchost()\n");
        res = usb_otg_control_enable_rchost(otg_core->otg_control);
        if(res != 0){
            /* 失敗したら usb_otg_control_enable_rchost() 発行済みでない 
               状態に戻す */
            clear_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags);
        }
    }
    
exit:
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    if(res != 0){
        PERR("usb_otg_control_enable_rchost()\n");
    }
    
    return 0;
}

void usb_otgcore_gadget_disconnect(void)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int res_testmode = 0, res_rchost = 0;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* TestMode中ならばNormalに戻す */
    if(otg_core->test_mode != USB_OTGCORE_TEST_MODE_NORMAL){
        res_testmode = usb_otg_control_set_test_mode(otg_core->otg_control, 
                                                     USB_OTG_TEST_MODE_NORMAL);
        if(res_testmode == 0){
            otg_core->test_mode = USB_OTGCORE_TEST_MODE_NORMAL;
        }
    }
    
    /* enable_rchost() 発行済みならばdisable_rchost() 呼ぶ */
    if(test_and_clear_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags)){
        res_rchost = usb_otg_control_disable_rchost(otg_core->otg_control);
    }
    
    /* Host によるhnp_enableをクリア */
    clear_bit(USB_OTGCORE_ATOMIC_HNP_ENABLE_HOST, &otg_core->bitflags);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    if(res_testmode != 0){
        PERR("usb_otg_control_set_test_mode()\n");
    }
    
    if(res_rchost != 0){
        PERR("usb_otg_control_disable_rchost()\n");
    }
    
    return;
}

int usb_otgcore_get_line_state( struct usb_otgcore_line_state *ls )
{
    
    int ret = 0;
    
    PDEBUG( "Calling: %s() ", __func__ );
    
    ret = do_get_line_state( ls );
    
    PDEBUG( "Leaving: %s() , return val is %d ", __func__, ret );
    
    return ret;
    
}

#if defined(CONFIG_ARCH_CXD90014BASED)
#define USB_OTGCORE_CXD90014_PORTSTSC_ADDR         ( 0xF0210104 )
#define USB_OTGCORE_CXD90014_PORTSTSC_LINESTATE_DP ( 0x00000020 )
#define USB_OTGCORE_CXD90014_PORTSTSC_LINESTATE_DM ( 0x00000040 )
#endif
static int do_get_line_state( struct usb_otgcore_line_state *ls )
{
    
    int ret = 0;
#if defined(CONFIG_ARCH_CXD90014BASED)
    UDIF_U32 reg_value = 0;
#endif
    
    PDEBUG( "Calling: %s() ", __func__ );
    
#if defined(CONFIG_ARCH_CXD4132BASED)
    struct otg_core_drv *otg_core = the_otg_core;
    struct usb_otg_line_state stLineState;
    
    // Get D+- line state from OTG controller driver
    usb_otg_control_ioctl( otg_core->otg_control, USB_IOCTL_GET_LINE_STATE, (unsigned long)&stLineState );
    
    // Check D+ line state
    if( stLineState.dp == USB_OTG_LINE_STATE_HIGH ){
        
        ls->dp = USB_OTGCORE_LINE_STATE_HIGH;
        
    }else{
        
        ls->dp = USB_OTGCORE_LINE_STATE_LOW;
        
    }
    
    // Check D- line state
    if( stLineState.dm == USB_OTG_LINE_STATE_HIGH ){
        
        ls->dm = USB_OTGCORE_LINE_STATE_HIGH;
        
    }else{
        
        ls->dm = USB_OTGCORE_LINE_STATE_LOW;
        
    }
#elif defined(CONFIG_ARCH_CXD90014BASED)
    // Get D+/- line state from MSS/KJR Macro
    reg_value = udif_ioread32( USB_OTGCORE_CXD90014_PORTSTSC_ADDR );
    
    // Check D+ line state
    if( ( reg_value & USB_OTGCORE_CXD90014_PORTSTSC_LINESTATE_DP ) == USB_OTGCORE_CXD90014_PORTSTSC_LINESTATE_DP ){
        
        ls->dp = USB_OTGCORE_LINE_STATE_HIGH;
        
    }else{
        
        ls->dp = USB_OTGCORE_LINE_STATE_LOW;
        
    }
    
    // Check D- line state
    if( ( reg_value & USB_OTGCORE_CXD90014_PORTSTSC_LINESTATE_DM ) == USB_OTGCORE_CXD90014_PORTSTSC_LINESTATE_DM ){
        
        ls->dm = USB_OTGCORE_LINE_STATE_HIGH;
        
    }else{
        
        ls->dm = USB_OTGCORE_LINE_STATE_LOW;
        
    }
#else
    ls->dp = USB_OTGCORE_LINE_STATE_LOW;
    ls->dm = USB_OTGCORE_LINE_STATE_LOW;
#endif
    
    PDEBUG( "Leaving: %s() , return val is %d ", __func__, ret );
    
    return ret;
    
}

/*-------------------------------------------------------------------------*/
static int
ioctl_probe(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct usb_otgcore_probe_info tmp_info;
    unsigned long flags;
    unsigned long res;
    
    PDEBUG("%s call\n", __func__);
    
    /* User空間からコピー */
    res = copy_from_user(&tmp_info, arg, sizeof(tmp_info));
    if(res != 0){
        PWARN("error: probe failed\n");
        return -EFAULT;
    }
    
    /* 引数が正しいかを確認 */
    if(tmp_info.hndl == 0){
        PWARN("error: handle is 0\n");
        return -EINVAL;
    }
    
    /* すでにprobe済みか確認 */
    if(otg_core->otg_probe.hndl != 0){
        PWARN("error: probe failed\n");
        /* すでにprobe済みならばエラー */
        return -EBUSY;
    }
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* tmpからコピー */
    memcpy(&otg_core->otg_probe,
           &tmp_info,
           sizeof(struct usb_otgcore_probe_info));
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    PDEBUG("otg_probe.hndl  :   %lu\n", otg_core->otg_probe.hndl);
    PDEBUG("otg_probe.event : 0x%lx\n", (unsigned long)&otg_core->otg_probe.event);
    PDEBUG(" event.cid : 0x%lx\n", (unsigned long)otg_core->otg_probe.event.cid);
    PDEBUG(" event.vbus: 0x%lx\n", (unsigned long)otg_core->otg_probe.event.vbus);
    PDEBUG(" event.vbus_error : 0x%lx\n", (unsigned long)otg_core->otg_probe.event.vbus_error);
    PDEBUG(" event.pullup : 0x%lx\n", (unsigned long)otg_core->otg_probe.event.pullup);
    PDEBUG(" event.receive_srp : 0x%lx\n", (unsigned long)otg_core->otg_probe.event.receive_srp);
    PDEBUG(" event.rchost : 0x%lx\n", (unsigned long)otg_core->otg_probe.event.rchost);
    PDEBUG(" event.rcgadget : 0x%lx\n", (unsigned long)otg_core->otg_probe.event.rcgadget);
    PDEBUG(" event.set_feature : 0x%lx\n", (unsigned long)otg_core->otg_probe.event.set_feature);
    
    // Notify Vbus Off and CID_B
    usb_otgcore_notify_vbusoff_cidb();
    
    // Notify current Vbus status.
    usb_otgcore_notify_current_vbus_state();
    
    // Notify current ID status.
    usb_otgcore_notify_current_id_state();
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_remove(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* Handleを0にする */
    otg_core->otg_probe.hndl = 0;
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int transition_to_stop(void)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int mode;
    int res;
    int i;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    /* IDLE状態であることを確認 */
    if(mode != USB_OTG_CONTROL_IDLE){
        PERR("mode error\n");
        res = -EBUSY;
        goto exit;
    }
    
    /* controllerを停止する */
    res = usb_otg_control_stop_control(otg_core->otg_control);
    if(res != 0){
        PERR("usb_otg_control_stop_control()\n");
    }
    usb_otgcore_free_vbusid_irq();
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 内部で保持しているVBUS状態をUNKNOWNに初期化 */
    for(i=0; i<(otg_core->port_num); i++){
        otg_core->port[i].vbus = USB_OTGCORE_VBUS_UNKNOWN;
    }

    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
exit:
    return res;
}

static int
ioctl_stop(void *arg)
{
    return transition_to_stop();
}

/*-------------------------------------------------------------------------*/
static int
transition_to_idle(void)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct m_otg_drv *tmp_otg_drv;
    unsigned long flags;
    int mode;
    int res = 0;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /* Gadget状態の時のみここでstopする */
    if(mode == USB_OTG_CONTROL_GADGET){
    
        /* usb_otg_control_enable_rchost() 発行済みならばここでdisableにする */
        if(test_and_clear_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags)){
            /* rchostを不許可にする */
            usb_otg_control_disable_rchost(otg_core->otg_control);
        }
        
        /* TestMode中 ならば解除する */
        if(otg_core->test_mode != USB_OTGCORE_TEST_MODE_NORMAL){
        
            /* TestModeをNORMALにする */
            PDEBUG("usb_otg_control_set_test_mode(USB_OTG_TEST_MODE_NORMAL)\n");
            res = usb_otg_control_set_test_mode(otg_core->otg_control, 
                                                USB_OTG_TEST_MODE_NORMAL);
            if(res == 0){
                otg_core->test_mode = USB_OTGCORE_TEST_MODE_NORMAL;
            }else{
                PERR("usb_otg_control_set_test_mode(USB_OTG_TEST_MODE_NORMAL)\n");
            }
        }
        
        /* Gadgetを終了すると hs_disable が DISABLE になる */
        otg_core->hs_disable = USB_OTGCORE_HS_DISABLE;
        
        /* Gadgetを終了する */
        res = usb_otg_control_stop_gadget(otg_core->otg_control);
    }
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    if(mode == USB_OTG_CONTROL_IDLE     ||
       mode == USB_OTG_CONTROL_HOST     ||
       mode == USB_OTG_CONTROL_RCHOST   ||
       mode == USB_OTG_CONTROL_RCGADGET   ){
        
        if(otg_core->test_mode != USB_OTGCORE_TEST_MODE_NORMAL         ||
           test_bit(USB_OTGCORE_ATOMIC_E_HOST, &otg_core->bitflags)   ){
            
            /* TestModeをNORMALにする */
            PDEBUG("usb_otg_control_set_test_mode(USB_OTG_TEST_MODE_NORMAL)\n");
            res = usb_otg_control_set_test_mode(otg_core->otg_control, 
                                                USB_OTG_TEST_MODE_NORMAL);
            if(res == 0){
                otg_core->test_mode = USB_OTGCORE_TEST_MODE_NORMAL;
                clear_bit(USB_OTGCORE_ATOMIC_E_HOST, &otg_core->bitflags);
            }else{
                PERR("usb_otg_control_set_test_mode(USB_OTG_TEST_MODE_NORMAL)\n");
            }
        }
    }
    
    /* モードに応じて処理をする */
    switch(mode){
      case USB_OTG_CONTROL_STOP:
        /* Controllerをstartする */
        res = usb_otg_control_start_control(otg_core->otg_control);
        usb_otgcore_request_vbusid_irq();
        break;
        
      case USB_OTG_CONTROL_IDLE:
        /* 何もしない */
        res = 0;
        break;
        
      case USB_OTG_CONTROL_GADGET:
        /* すでにstopしているので何もしない */
        break;
        
      case USB_OTG_CONTROL_HOST:
        /* Hostを停止する */
        res = usb_otg_control_stop_host(otg_core->otg_control);
        
        /***** スピンロック *****/
        spin_lock_irqsave(&otg_core->lock, flags);
        
        /* リストからOTG Driverを取り出してnotifyを呼ぶ */
        list_for_each_entry(tmp_otg_drv, &otg_core->drv_list, list){
            if(tmp_otg_drv->otg_drv->notify){
                PDEBUG("notify(USB_OTGCORE_STOP_HOST) to %p\n", tmp_otg_drv->otg_drv);
                tmp_otg_drv->otg_drv->notify(tmp_otg_drv->otg_drv, USB_OTGCORE_STOP_HOST);
            }
        }
        
        /***** スピンロック解除 *****/
        spin_unlock_irqrestore(&otg_core->lock, flags);

        break;
        
      case USB_OTG_CONTROL_RCGADGET:
        /* RCGadgetを停止する */
        res = usb_otg_control_stop_rcgadget(otg_core->otg_control, 0);
        break;
        
      case USB_OTG_CONTROL_RCHOST:
        /* RCHostを停止する */
        res = usb_otg_control_stop_rchost(otg_core->otg_control);
        break;
        
      default:
        res = -EINVAL;
        break;
    }

    /* for PHY suspend */
    if( USB_OTG_CONTROL_IDLE == usb_otg_control_get_mode(otg_core->otg_control)){
    	set_phy_suspend();
    }

    if(res != 0){
        PERR("transition to IDLE mode\n");
    }
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_idle(void *arg)
{
    return transition_to_idle();
}

/*-------------------------------------------------------------------------*/
#if defined(CONFIG_ARCH_CXD90014BASED)
  #define USB_OTGCORE_CXD90014_USB_REGISTER_USB_VBUS_UNLIMIT_REGISTER     ( 0xF2920000 )
  #define USB_OTGCORE_CXD90014_USB_DEVICE_CONF_REGISTER                   ( 0xF0210000 )
  #define USB_OTGCORE_CXD90014_USB_DEVICE_MODE_REGISTER                   ( 0xF0210004 )
  #define USB_OTGCORE_CXD90014_USB_DEVICE_DEVS_REGISTER                   ( 0xF0210204 )
  #define USB_OTGCORE_CXD90014_USB_USB_HF_SEL_BIT                         ( 0x00000010 ) //bit4:USB_HF_SEL
  #define USB_OTGCORE_CXD90014_USB_DEVICE_CONF_SOFTRESET_BIT              ( 0x00000004 ) //bit2:softreset
  #define USB_OTGCORE_CXD90014_USB_DEVICE_MODE_DEV_EN_BIT                 ( 0x00000002 ) //bit1:dev_en
  #define USB_OTGCORE_CXD90014_USB_DEVICE_DEVS_SUSPEND_BIT                ( 0x00000001 ) //bit0:suspend
#endif

static void set_phy_suspend( void )
{
	unsigned long val = 0;

	val = udif_ioread32( USB_OTGCORE_CXD90014_USB_REGISTER_USB_VBUS_UNLIMIT_REGISTER );
	val &= ~USB_OTGCORE_CXD90014_USB_USB_HF_SEL_BIT; /* write 0 to bit4 */
	udif_iowrite32( val, USB_OTGCORE_CXD90014_USB_REGISTER_USB_VBUS_UNLIMIT_REGISTER );

	val = udif_ioread32( USB_OTGCORE_CXD90014_USB_DEVICE_CONF_REGISTER );
	val |= USB_OTGCORE_CXD90014_USB_DEVICE_CONF_SOFTRESET_BIT; /* write 1 to bit2 */
	udif_iowrite32( val, USB_OTGCORE_CXD90014_USB_DEVICE_CONF_REGISTER );

polling:
	
	val = udif_ioread32( USB_OTGCORE_CXD90014_USB_DEVICE_CONF_REGISTER );
	val &= USB_OTGCORE_CXD90014_USB_DEVICE_CONF_SOFTRESET_BIT; /* read bit2 only */

	if(val != 0){
		PDEBUG("soft reset waiting...\n");
		/* bit4:It is ensured that soft reset bit turns zero after 4 cycles... */
		goto polling;
	}

	val = udif_ioread32( USB_OTGCORE_CXD90014_USB_DEVICE_MODE_REGISTER );
	val |= USB_OTGCORE_CXD90014_USB_DEVICE_MODE_DEV_EN_BIT; /* write 1 to bit1 */
	udif_iowrite32( val, USB_OTGCORE_CXD90014_USB_DEVICE_MODE_REGISTER );

wait_suspend:

	val = udif_ioread32( USB_OTGCORE_CXD90014_USB_DEVICE_DEVS_REGISTER );
	val &= USB_OTGCORE_CXD90014_USB_DEVICE_DEVS_SUSPEND_BIT; /* read bit0 only */
	if( val != 1 ){
		PDEBUG("wait_suspend...\n");
		goto wait_suspend;
	}

	/* Set Phy Params */
	set_phy_params();

	return;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_start_gadget(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int mode;
    int res;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    /* 現在の状態がIDLEか確認する */
    if(mode != USB_OTG_CONTROL_IDLE){
        PERR("mode error\n");
        res = -EBUSY;
        goto exit;
    }
    
    /* Gadget を開始する */
    res = usb_otg_control_start_gadget(otg_core->otg_control);
    if(res != 0){
        PERR("transfer to Gadget\n");
        goto exit;
    }
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* Gadgetを開始すると hs_disable が ENABLE になる */
    otg_core->hs_disable = USB_OTGCORE_HS_ENABLE;
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
exit:
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_start_host(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct m_otg_drv *tmp_otg_drv;
    unsigned long flags;
    int mode;
    int res;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    /* 現在の状態がIDLEか確認する */
    if(mode != USB_OTG_CONTROL_IDLE){
        PERR("mode error\n");
        res = -EBUSY;
        goto exit;
    }
    
    /* Host を開始する */
    res = usb_otg_control_start_host(otg_core->otg_control);
    if(res != 0){
        PERR("transfer to Host\n");
        goto exit;
    }
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* リストからOTG Driverを取り出してnotifyを呼ぶ */
    list_for_each_entry(tmp_otg_drv, &otg_core->drv_list, list){
        if(tmp_otg_drv->otg_drv->notify){
            PDEBUG("notify(USB_OTGCORE_START_HOST) to %p\n", tmp_otg_drv->otg_drv);
            tmp_otg_drv->otg_drv->notify(tmp_otg_drv->otg_drv, USB_OTGCORE_START_HOST);
        }
    }
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
exit:
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_start_e_host(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int mode;
    int res = 0;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    /* IDLE状態であることを確認 */
    if(mode != USB_OTG_CONTROL_IDLE){
        PERR("mode error\n");
        res = -EBUSY;
        goto exit;
    }
    
    res = usb_otg_control_set_test_mode(otg_core->otg_control, 
                                        USB_OTG_TEST_MODE_H_AS);
    if(res != 0){
        PERR("usb_otg_control_set_test_mode(USB_OTG_TEST_MODE_H_AS)\n");
        goto exit;
    }
    
    /* Host を開始する */
    res = usb_otg_control_start_host(otg_core->otg_control);
    if(res != 0){
        PERR("transfer to Host\n");
        goto exit;
    }
    
    set_bit(USB_OTGCORE_ATOMIC_E_HOST, &otg_core->bitflags);
    
exit:
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_enable_rchost(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int mode;
    int res = 0;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /* 現在の状態がGadgetか確認する */
    if(mode != USB_OTG_CONTROL_GADGET){
        PERR("mode error\n");
        res = -EBUSY;
        goto exit;
    }
    
    /* UserによりHNPが許可されたことを保存 */
    set_bit(USB_OTGCORE_ATOMIC_HNP_ENABLE_USER, &otg_core->bitflags);
    
    /* HostからHNP が許可されている &&
       usb_otg_control_enable_rchost() 発行済みでない    */
    if(test_bit(USB_OTGCORE_ATOMIC_HNP_ENABLE_HOST, &otg_core->bitflags) &&
       test_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags) == 0){
        
        /* rchostを許可する */
        PDEBUG("usb_otg_control_enable_rchost()\n");
        res = usb_otg_control_enable_rchost(otg_core->otg_control);
        if(res == 0){
            /* 成功したら usb_otg_control_enable_rchost() 発行済み状態にする */
            set_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags);
        }
    }
    
exit:
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_disable_rchost(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int mode;
    int res = 0;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /* 現在の状態がGadgetか確認する */
    if(mode != USB_OTG_CONTROL_GADGET){
        PERR("mode error\n");
        res = -EBUSY;
        
        goto exit;
    }
    
    /* UserによりHNPが不許可にされたことを保存 */
    clear_bit(USB_OTGCORE_ATOMIC_HNP_ENABLE_USER, &otg_core->bitflags);
    
    /* usb_otg_control_enable_rchost() 発行済み    */
    if(test_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags)){
        
        /* rchostを不許可にする */
        PDEBUG("usb_otg_control_disable_rchost()\n");
        res = usb_otg_control_disable_rchost(otg_core->otg_control);
        if(res == 0){
            /* 成功したら usb_otg_control_enable_rchost() 発行済み状態を解除する */
            clear_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags);
        }
    }
    
exit:
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_start_rcgadget(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int mode;
    int res;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    /* 現在の状態がHOSTか確認する */
    if(mode != USB_OTG_CONTROL_HOST){
        PERR("mode error\n");
        res = -EBUSY;
        goto exit;
    }
    
    /* RCGadget を開始する */
    res = usb_otg_control_start_rcgadget(otg_core->otg_control, 0);
    if(res != 0){
        PERR("transfer to Gadget\n");
    }
    
exit:
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_set_speed(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    unsigned char otg_core_speed;
    unsigned int  ctrl_speed;
    
    PDEBUG("%s call\n", __func__);
    
    /* User空間からコピー */
    if(copy_from_user(&otg_core_speed, arg, sizeof(otg_core_speed))){
        return -EFAULT;
    }
    
    /* OTG Core の値から OTG Controller の値へ変換 */
    switch(otg_core_speed){
      case USB_OTGCORE_HS_ENABLE:
        ctrl_speed = USB_OTG_HS_ENABLE;
        break;
        
      case USB_OTGCORE_HS_DISABLE:
        ctrl_speed = USB_OTG_HS_DISABLE;
        break;
        
      default:
        return -EINVAL;
    }
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    otg_core->hs_disable = otg_core_speed;
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    /* Controller を呼んでspeedを設定 */
    return usb_otg_control_set_speed(otg_core->otg_control, ctrl_speed);
}

/*-------------------------------------------------------------------------*/
static int
ioctl_get_speed(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    enum usb_otg_control_speed ctrl_speed;
    unsigned char otg_core_speed;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* OTG Controller から BusSpeedを取得 */
    ctrl_speed = usb_otg_control_get_speed(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    /* OTG Controller の値から OTG Core の値へ変換 */
    switch(ctrl_speed){
      case USB_OTG_SPEED_LS:
        otg_core_speed = USB_OTGCORE_SPEED_LS;
        break;
        
      case USB_OTG_SPEED_FS:
        otg_core_speed = USB_OTGCORE_SPEED_FS;
        break;
        
      case USB_OTG_SPEED_HS:
        otg_core_speed = USB_OTGCORE_SPEED_HS;
        break;
      
      default:
        return -EFAULT;
    }
    
    /* User空間にコピー */
    if(copy_to_user(arg, &otg_core_speed, sizeof(otg_core_speed))){
        return -EFAULT;
    }
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_set_test_mode(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned char tmp_mode;
    int res;
    enum usb_otg_control_test_mode test_mode;
    
    PDEBUG("%s call\n", __func__);
    
    /* User空間からコピー */
    res = copy_from_user(&tmp_mode, 
                         (unsigned char*)arg,
                         sizeof(unsigned char));
    if(res != 0){
        PERR("error: copy_from_user\n");
        return -EFAULT;
    }
    
    res = 0;
    switch(tmp_mode){
      case USB_OTGCORE_TEST_MODE_NORMAL:
        PDEBUG("USB_OTG_TEST_MODE_NORMAL\n");
        test_mode = USB_OTG_TEST_MODE_NORMAL;
        break;
        
      case USB_OTGCORE_TEST_MODE_G_PACKET:
        PDEBUG("USB_OTG_TEST_MODE_G_PACKET\n");
        test_mode = USB_OTG_TEST_MODE_G_PACKET;
        break;
        
      case USB_OTGCORE_TEST_MODE_G_K:
        PDEBUG("USB_OTG_TEST_MODE_G_K\n");
        test_mode = USB_OTG_TEST_MODE_G_K;
        break;
        
      case USB_OTGCORE_TEST_MODE_G_J:
        PDEBUG("USB_OTG_TEST_MODE_G_J\n");
        test_mode = USB_OTG_TEST_MODE_G_J;
        break;
        
      case USB_OTGCORE_TEST_MODE_G_SE0_NAK:
        PDEBUG("USB_OTG_TEST_MODE_G_SE0_NAK\n");
        test_mode = USB_OTG_TEST_MODE_G_SE0_NAK;
        break;
        
      case USB_OTGCORE_TEST_MODE_G_FORCE_PACKET:
        PDEBUG("USB_OTG_TEST_MODE_G_FORCE_PACKET\n");
        test_mode = USB_OTG_TEST_MODE_G_FORCE_PACKET;
        break;
        
      case USB_OTGCORE_TEST_MODE_H_LS:
        PDEBUG("USB_OTG_TEST_MODE_H_LS\n");
        test_mode = USB_OTG_TEST_MODE_H_LS;
        break;
        
      case USB_OTGCORE_TEST_MODE_H_FS:
        PDEBUG("USB_OTG_TEST_MODE_H_FS\n");
        test_mode = USB_OTG_TEST_MODE_H_FS;
        break;
        
      case USB_OTGCORE_TEST_MODE_H_HS:
        PDEBUG("USB_OTG_TEST_MODE_H_HS\n");
        test_mode = USB_OTG_TEST_MODE_H_HS;
        break;
        
      default:
        PDEBUG("Error\n");
        res = -EINVAL;
        break;
    }
    
    if(res == 0){
        res = usb_otg_control_set_test_mode(otg_core->otg_control, test_mode);
        if(res == 0){
            otg_core->test_mode = tmp_mode;
        }
    }
    
    if(res != 0){
        PWARN("error: TestMode\n");
    }
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_send_srp(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    // struct usb_otg_control_port_info port_info; <- 使わない
    unsigned long flags;
    int mode;
    int res;
    
    PDEBUG("%s call\n", __func__);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /* IDLE状態であることを確認 */
    if(mode != USB_OTG_CONTROL_IDLE){
        /***** スピンロック解除 *****/
        spin_unlock_irqrestore(&otg_core->lock, flags);
        
        PERR("mode error\n");
        res = -EBUSY;
        goto exit;
    }
    
    /* Port数、current portを確認 */
    // usb_otg_control_get_port_info(otg_core->otg_control,&port_info); <- 使わない
    if(otg_core->port_num == USB_OTGCORE_PORT_NOSET || otg_core->port_num < the_port_info.current_port ){
        res = -EINVAL;
        /***** スピンロック解除 *****/
        spin_unlock_irqrestore(&otg_core->lock, flags);
        goto exit;
    }
    
    /* CIDがBであることを確認 */
    if(otg_core->port[the_port_info.current_port].cid != USB_OTGCORE_CID_B){
        /***** スピンロック解除 *****/
        spin_unlock_irqrestore(&otg_core->lock, flags);
        
        PERR("cid error\n");
        res = -EBUSY;
        goto exit;
    }
    
    /* SRPパルシングを要求 */
    res = usb_otg_control_request_session(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    if(res != 0){
        PERR("usb_otg_control_request_session()\n");
        res = -EINVAL;
        goto exit;
    }
    
exit:
    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_select_port(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned int port;
    int res = 0;
    
    PDEBUG("%s call\n", __func__);

    /* User空間からコピー */
    res = copy_from_user(&port, arg, sizeof(port));

    if(res != 0){
        PERR("error: copy_from_user\n");
        return -EFAULT;
    }

    if(otg_core->port_num == USB_OTGCORE_PORT_NOSET || otg_core->port_num == 1){
        /* デスクリプタ未登録か、登録されたデスクリプタのポート数が1つの場合は、 */
        /* 何もせずに正常終了する  */
        PDEBUG("Nothing to do. \n");
        return res;
    }
    
    if(otg_core->port_num < port){
        PERR("%s error: invalid port: %d\n", __func__, otg_core->port_num);
        return -EINVAL;
    }
    
    PDEBUG("%s port: %d\n", __func__, port);
    // res = usb_otg_control_select_port(otg_core->otg_control, port); <- 使わない
    gpio_set_data_bit(the_port_desc.select.gpio_port, the_port_desc.select.gpio_bit, port);
    the_port_info.current_port = port;

    return res;
}

/*-------------------------------------------------------------------------*/
static int
ioctl_getPortInfo(void *arg)
{
    // struct otg_core_drv *otg_core = the_otg_core; <- 使わない
    // struct usb_otg_control_port_info port_info; <- 使わない
    struct usb_otgcore_port_info otg_port_info;    

    PDEBUG("%s call\n", __func__);
    
    /* USB port数を取得 */
    // usb_otg_control_get_port_info(otg_core->otg_control,&port_info); <- 使わない
    
    /* OTG Controller の値から OTG Core の値へ変換 */
    switch(the_port_info.nr){
      case 0:
        otg_port_info.nr = USB_OTGCORE_PORT_NOSET;
        break;
        
      case 1:
        otg_port_info.nr = USB_OTGCORE_PORT_SINGLE;
        break;
        
      case 2:
        otg_port_info.nr = USB_OTGCORE_PORT_DOUBLE;
        break;
        
      default:
        return -EFAULT;
    }

    switch(the_port_info.current_port){
      case 0:
        otg_port_info.current_port = USB_OTGCORE_PORT0;
        break;
        
      case 1:
        otg_port_info.current_port = USB_OTGCORE_PORT1;
        break;
        
      default:
        return -EFAULT;
    }
    
    PDEBUG("%s nr: %d current_port:%d\n", __func__, port_info.nr, port_info.current_port);
    
    /* User空間にコピー */
    if(copy_to_user(arg, &otg_port_info, sizeof(otg_port_info))){
        return -EFAULT;
    }
    return 0;
}

/*-------------------------------------------------------------------------*/
static int conv_io_type_otgcore2ups( int type )
{
    int ups_type = 0;
    switch( type ){
      case USB_OTGCORE_IO_TYPE_HFIX:
        ups_type = UPS_IO_TYPE_HFIX;
        break;
      case USB_OTGCORE_IO_TYPE_LFIX:
        ups_type = UPS_IO_TYPE_LFIX;
        break;
      case USB_OTGCORE_IO_TYPE_MISC:
        ups_type = UPS_IO_TYPE_MISC;
        break;
      case USB_OTGCORE_IO_TYPE_GPIO:
        ups_type = UPS_IO_TYPE_GPIO;
        break;
      case USB_OTGCORE_IO_TYPE_GPIOX:
        ups_type = UPS_IO_TYPE_GPIOX;
        break;
      default:
        PERR("error");
        break;
    }
    return ups_type;
}

static int
ioctl_registPortDescriptor(void *arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct usb_otgcore_port_descriptor otg_port_desc;
    int res;
    int i;

    PDEBUG("%s call\n", __func__);

    /* Copy parameters from userspace. */
    res = copy_from_user(&otg_port_desc, arg, sizeof(struct usb_otgcore_port_descriptor));

    if( 0 != res ){
        PERR("error: copy_from_user\n");
        return -EFAULT;
    }

    /* Set parameters into internal variable. */
    PDEBUG("usb_otgcore_port_descriptor {\n"
               "  port_num                = %d\n",otg_port_desc.port_num);
    the_port_desc.port_num = otg_port_desc.port_num;
    for ( i = 0; i < otg_port_desc.port_num; i++){
        PDEBUG("  port[%d].cid .type      = %d\n",i, otg_port_desc.port[i].cid.type);
        the_port_desc.port[i].cid.type = conv_io_type_otgcore2ups(otg_port_desc.port[i].cid.type);

        PDEBUG("               .gpio_port = %d\n", otg_port_desc.port[i].cid.gpio_port);
        the_port_desc.port[i].cid.gpio_port = otg_port_desc.port[i].cid.gpio_port;

        PDEBUG("               .gpio_bit  = %d\n", otg_port_desc.port[i].cid.gpio_bit);
        the_port_desc.port[i].cid.gpio_bit = otg_port_desc.port[i].cid.gpio_bit;
            
        PDEBUG("           vbus.type      = %d\n", otg_port_desc.port[i].vbus.type);
        the_port_desc.port[i].vbus.type = conv_io_type_otgcore2ups(otg_port_desc.port[i].vbus.type);
        
        PDEBUG("               .gpio_port = %d\n", otg_port_desc.port[i].vbus.gpio_port);
        the_port_desc.port[i].vbus.gpio_port = otg_port_desc.port[i].vbus.gpio_port;
        
        PDEBUG("               .gpio_bit  = %d\n", otg_port_desc.port[i].vbus.gpio_bit);
        the_port_desc.port[i].vbus.gpio_bit = otg_port_desc.port[i].vbus.gpio_bit;
    }
    PDEBUG(    "  select.type             = %d\n", otg_port_desc.select.type);
    the_port_desc.select.type = otg_port_desc.select.type;
    
    PDEBUG(    "        .gpio_port        = %d\n", otg_port_desc.select.gpio_port);
    the_port_desc.select.gpio_port = otg_port_desc.select.gpio_port;
    
    PDEBUG(    "        .gpio_bit         = %d\n", otg_port_desc.select.gpio_bit);
    the_port_desc.select.gpio_bit = otg_port_desc.select.gpio_bit;

    /* Update port status. */
    otg_core->port_num = otg_port_desc.port_num;
    the_port_info.nr = otg_port_desc.port_num;
    
    for (i=0; i<(otg_port_desc.port_num); i++){
        otg_core->port[i].id = i;
        otg_core->port[i].vbus = USB_OTGCORE_VBUS_UNKNOWN;
    }
    return 0;
}

static int
ioctl_unregistPortDescriptor(void)
{
    
    PDEBUG("%s call\n", __func__);
    return 0;
}

#if defined(CONFIG_ARCH_CXD90014BASED)
  #define USB_OTGCORE_CXD90014_HOST_PHY_PARAM_ADDR   ( 0xF020200C )
  #define USB_OTGCORE_CXD90014_DEVICE_PHY_PARAM_ADDR ( 0xF0210018 )
  #define USB_OTGCORE_CXD90014_HSDTRSEL_MASK         ( 0x00000700 )
  #define USB_OTGCORE_CXD90014_ENVDETSEL_MASK        ( 0x00000070 )
  #define USB_OTGCORE_CXD90014_HSDSEL_MASK           ( 0x00000007 )
  #define USB_OTGCORE_CXD90014_PHY_PARAM_MASK        ( 0x00000777 )
#endif

static int
ioctl_setPhyParam( void *arg )
{
    //struct usb_otgcore_phy_param st_phy_param;
    int res = 0;
    UDIF_U32 host_phy_value = 0;
    UDIF_U32 device_phy_value = 0;
    
    PDEBUG( "%s call\n", __func__ );
    
    /* Copy PHY parameters from userspace. */
    res = copy_from_user( &st_phy_param, arg, sizeof( struct usb_otgcore_phy_param ) );
    
    if( 0 != res ){
        
        PERR( "error: copy_from_user\n" );
        return -EFAULT;
        
    }

    /* move following process to set_phy_params() */
#if 0
#if defined(CONFIG_ARCH_CXD90014BASED)
    
    /* Check delivered PHY Value */
    PDEBUG( "st_phy_param.phy_data[0] is 0x%02X \n", st_phy_param.phy_data[0] );
    PDEBUG( "st_phy_param.phy_data[1] is 0x%02X \n", st_phy_param.phy_data[1] );
    PDEBUG( "st_phy_param.phy_data[2] is 0x%02X \n", st_phy_param.phy_data[2] );
    PDEBUG( "st_phy_param.phy_data[3] is 0x%02X \n", st_phy_param.phy_data[3] );
    
    /* --- Set Host PHY params --- */
    // 1st. Read current PHY value.
    host_phy_value = udif_ioread32( USB_OTGCORE_CXD90014_HOST_PHY_PARAM_ADDR );
    PDEBUG( "Current Host PHY value is 0x%08X \n", host_phy_value );
    
    // 2nd. Escape values other than PHY parameters.
    host_phy_value &= ~USB_OTGCORE_CXD90014_PHY_PARAM_MASK;
    PDEBUG( "Escaped value is 0x%08X \n", host_phy_value );
    
    // 3rd. Set ENVDETSEL/HSDSEL value.
    host_phy_value += ( st_phy_param.phy_data[0] & 
                        ( USB_OTGCORE_CXD90014_ENVDETSEL_MASK | USB_OTGCORE_CXD90014_HSDSEL_MASK ) );
    PDEBUG( "Value after setting ENVDETSEL/HSDSEL is 0x%08X \n", host_phy_value );
    
    // 4th. Set HSDTRSEL value.
    host_phy_value += ( ( st_phy_param.phy_data[1] << 8 ) & USB_OTGCORE_CXD90014_HSDTRSEL_MASK );
    PDEBUG( "Value after setting HSDTRSEL is 0x%08X \n", host_phy_value );
    
    // Finally, write.
    udif_iowrite32( host_phy_value, USB_OTGCORE_CXD90014_HOST_PHY_PARAM_ADDR );
    
    /* --- Set Host Device params --- */
    // 1st. Read current PHY value.
    device_phy_value = udif_ioread32( USB_OTGCORE_CXD90014_DEVICE_PHY_PARAM_ADDR );
    PDEBUG( "Current Device PHY value is 0x%08X \n", device_phy_value );
    
    // 2nd. Escape values other than PHY parameters.
    device_phy_value &= ~USB_OTGCORE_CXD90014_PHY_PARAM_MASK;
    PDEBUG( "Escaped value is 0x%08X \n", device_phy_value );
    
    // 3rd. Set ENVDETSEL/HSDSEL value.
    device_phy_value += ( st_phy_param.phy_data[2] & 
                        ( USB_OTGCORE_CXD90014_ENVDETSEL_MASK | USB_OTGCORE_CXD90014_HSDSEL_MASK ) );
    PDEBUG( "Value after setting ENVDETSEL/HSDSEL is 0x%08X \n", device_phy_value );
    
    // 4th. Set HSDTRSEL value.
    device_phy_value += ( ( st_phy_param.phy_data[3] << 8 ) & USB_OTGCORE_CXD90014_HSDTRSEL_MASK );
    PDEBUG( "Value after setting HSDTRSEL is 0x%08X \n", device_phy_value );
    
    // Finally, write.
    udif_iowrite32( device_phy_value, USB_OTGCORE_CXD90014_DEVICE_PHY_PARAM_ADDR );
#endif
#endif
    return 0;
    
}

/*-------------------------------------------------------------------------*/
static void set_phy_params( void )
{
    UDIF_U32 host_phy_value = 0;
    UDIF_U32 device_phy_value = 0;
    
#if defined(CONFIG_ARCH_CXD90014BASED)
    
	/* Check delivered PHY Value */
    PDEBUG( "st_phy_param.phy_data[0] is 0x%02X \n", st_phy_param.phy_data[0] );
    PDEBUG( "st_phy_param.phy_data[1] is 0x%02X \n", st_phy_param.phy_data[1] );
    PDEBUG( "st_phy_param.phy_data[2] is 0x%02X \n", st_phy_param.phy_data[2] );
    PDEBUG( "st_phy_param.phy_data[3] is 0x%02X \n", st_phy_param.phy_data[3] );
    
    /* --- Set Host PHY params --- */
    // 1st. Read current PHY value.
    host_phy_value = udif_ioread32( USB_OTGCORE_CXD90014_HOST_PHY_PARAM_ADDR );
    PDEBUG( "Current Host PHY value is 0x%08X \n", host_phy_value );
    
    // 2nd. Escape values other than PHY parameters.
    host_phy_value &= ~USB_OTGCORE_CXD90014_PHY_PARAM_MASK;
    PDEBUG( "Escaped value is 0x%08X \n", host_phy_value );
    
    // 3rd. Set ENVDETSEL/HSDSEL value.
    host_phy_value += ( st_phy_param.phy_data[0] & 
                        ( USB_OTGCORE_CXD90014_ENVDETSEL_MASK | USB_OTGCORE_CXD90014_HSDSEL_MASK ) );
    PDEBUG( "Value after setting ENVDETSEL/HSDSEL is 0x%08X \n", host_phy_value );
    
    // 4th. Set HSDTRSEL value.
    host_phy_value += ( ( st_phy_param.phy_data[1] << 8 ) & USB_OTGCORE_CXD90014_HSDTRSEL_MASK );
    PDEBUG( "Value after setting HSDTRSEL is 0x%08X \n", host_phy_value );
    
    // Finally, write.
    udif_iowrite32( host_phy_value, USB_OTGCORE_CXD90014_HOST_PHY_PARAM_ADDR );
    
    /* --- Set Host Device params --- */
    // 1st. Read current PHY value.
    device_phy_value = udif_ioread32( USB_OTGCORE_CXD90014_DEVICE_PHY_PARAM_ADDR );
    PDEBUG( "Current Device PHY value is 0x%08X \n", device_phy_value );
    
    // 2nd. Escape values other than PHY parameters.
    device_phy_value &= ~USB_OTGCORE_CXD90014_PHY_PARAM_MASK;
    PDEBUG( "Escaped value is 0x%08X \n", device_phy_value );
    
    // 3rd. Set ENVDETSEL/HSDSEL value.
    device_phy_value += ( st_phy_param.phy_data[2] & 
                        ( USB_OTGCORE_CXD90014_ENVDETSEL_MASK | USB_OTGCORE_CXD90014_HSDSEL_MASK ) );
    PDEBUG( "Value after setting ENVDETSEL/HSDSEL is 0x%08X \n", device_phy_value );
    
    // 4th. Set HSDTRSEL value.
    device_phy_value += ( ( st_phy_param.phy_data[3] << 8 ) & USB_OTGCORE_CXD90014_HSDTRSEL_MASK );
    PDEBUG( "Value after setting HSDTRSEL is 0x%08X \n", device_phy_value );
    
    // Finally, write.
    udif_iowrite32( device_phy_value, USB_OTGCORE_CXD90014_DEVICE_PHY_PARAM_ADDR );
#endif

	return;
}

/*-------------------------------------------------------------------------*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long
usb_otgcore_ioctl( struct file *filp,
#else
static int
usb_otgcore_ioctl(struct inode *inode,
                      struct file *filp,
#endif
                      unsigned int cmd,
                      unsigned long arg)
{
    struct otg_core_drv *otg_core = the_otg_core;
    int res;
    void __user *argp = (void __user *)arg;
    
    /* セマフォ取得 */
    udif_mutex_lock(&otg_core->sem);
    
    /* bind済みか確認する */
    if(test_bit(USB_OTGCORE_ATOMIC_BIND, &otg_core->bitflags) == 0){
        /* セマフォ開放 */
        udif_mutex_unlock(&otg_core->sem);
        
        PERR("not bind\n");
        return -EBUSY;
    }
    
    switch(cmd){
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_PROBE:
        res = ioctl_probe(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_REMOVE:
        res = ioctl_remove(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_STOP:
        res = ioctl_stop(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_IDLE:
        res = ioctl_idle(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_START_GADGET:
        res = ioctl_start_gadget(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_START_HOST:
        res = ioctl_start_host(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_START_E_HOST:
        res = ioctl_start_e_host(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_ENABLE_RCHOST:
        res = ioctl_enable_rchost(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_DISABLE_RCHOST:
        res = ioctl_disable_rchost(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_START_RCGADGET:
        res = ioctl_start_rcgadget(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_SET_SPEED:
        res = ioctl_set_speed(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_GET_SPEED:
        res = ioctl_get_speed(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_SET_TEST_MODE:
        res = ioctl_set_test_mode(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_SEND_SRP:
        res = ioctl_send_srp(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_SELECT_PORT:
        res = ioctl_select_port(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_GET_PORTINFO:
        res = ioctl_getPortInfo(argp);
        break;
        
      /*----------------------------------------*/
      case USB_IOC_OTGCORE_REGIST_PORT_DESCRIPTOR:
        res = ioctl_registPortDescriptor(argp);
        break;

      /*----------------------------------------*/
      case USB_IOC_OTGCORE_UNREGIST_PORT_DESCRIPTOR:
        res = ioctl_unregistPortDescriptor();
        break;

      /*----------------------------------------*/
      case USB_IOC_OTGCORE_SET_PHY_PARAM:
        res = ioctl_setPhyParam( argp );
        break;
        
      /*----------------------------------------*/
      default:
        PWARN("usb_otgcore_ioctl(none) call\n");
        res = -ENOTTY;
    }
    
    /* セマフォ開放 */
    udif_mutex_unlock(&otg_core->sem);
    
    return res;
}

#if defined(CONFIG_ARCH_CXD90014BASED)
  #define USB_OTGCORE_CXD90014_USB_INTHE_ADDR   ( 0xF2920040 )
  #define USB_OTGCORE_CXD90014_USB_INTLE_ADDR   ( 0xF2920050 )
  #define USB_OTGCORE_CXD90014_USB_VBUS_ID_MASK ( 0x00000300 )
  #define USB_OTGCORE_CXD90014_USB_VBUS_EDGE_EN ( 0x00000100 )
  #define USB_OTGCORE_CXD90014_USB_ID_EDGE_EN   ( 0x00000200 )
#endif
/*-------------------------------------------------------------------------*/
static int
usb_otgcore_open(struct inode *inode, struct file *filp)
{
    struct otg_core_drv *otg_core = the_otg_core;
    // struct usb_otg_control_port_info port_info; <- 使わない
    int i;
#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
    UDIF_U32 reg_value = 0;
#endif
    /* 他のprocessがopenしていないか確認 */
    if(test_and_set_bit(USB_OTGCORE_ATOMIC_FD_LOCK, &otg_core->bitflags) != 0){
        PWARN("usb_otgcore_open() failed\n");
        return -EBUSY;
    }

    /* port数を初期化 */
    otg_core->port_num = USB_OTGCORE_PORT_NOSET;
    
    /* USB port数を取得(ここは0しか返らない) */
    // usb_otg_control_get_port_info(otg_core->otg_control,&port_info); <- 使わない
    if(the_port_info.nr <= 0){
        the_port_info.nr = 1;
    }

#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
 #if defined(CONFIG_ARCH_CXD90014BASED)
    /* Initialize Registers */
    
    /* USB_INTHE */
    // 1st. Read current register.
    reg_value = udif_ioread32( USB_OTGCORE_CXD90014_USB_INTHE_ADDR );
    PDEBUG( "Current INTHE register value is 0x%08X \n", reg_value );
    
    // 2nd. Escape register.
    reg_value &= ~USB_OTGCORE_CXD90014_USB_VBUS_ID_MASK;
    PDEBUG( "Escaped register value is 0x%08X \n", reg_value );
    
    // 3rd. Set edge enable.
    reg_value += USB_OTGCORE_CXD90014_USB_VBUS_EDGE_EN + 
                    USB_OTGCORE_CXD90014_USB_ID_EDGE_EN;
    PDEBUG( "Value after setting edge is 0x%08X \n", reg_value );
    
    // Finally, write.
    udif_iowrite32( reg_value, USB_OTGCORE_CXD90014_USB_INTHE_ADDR );
    
    /* USB_INTLE */
    reg_value = udif_ioread32( USB_OTGCORE_CXD90014_USB_INTLE_ADDR );
    PDEBUG( "Current INTLE register value is 0x%08X \n", reg_value );
    
    // 2nd. Escape register.
    reg_value &= ~USB_OTGCORE_CXD90014_USB_VBUS_ID_MASK;
    PDEBUG( "Escaped register value is 0x%08X \n", reg_value );
    
    // 3rd. Set edge enable.
    reg_value += USB_OTGCORE_CXD90014_USB_VBUS_EDGE_EN + 
                    USB_OTGCORE_CXD90014_USB_ID_EDGE_EN;
    PDEBUG( "Value after setting edge is 0x%08X \n", reg_value );
    
    // Finally, write.
    udif_iowrite32( reg_value, USB_OTGCORE_CXD90014_USB_INTLE_ADDR );
 #endif
#endif

    // ここは1しか入らないので、要注意
    otg_core->port_num = the_port_info.nr;
    
    // usb_otgcore_module_init()でallocしたポート数分初期化する
    for(i=0; i<USB_OTGCORE_PORT_MAX; i++){
        /* ポートIDの設定 */
        otg_core->port[i].id = i;
        /* VBUS状態をUNKNOWNに初期化 */
        otg_core->port[i].vbus = USB_OTGCORE_VBUS_UNKNOWN;
    }
    
    PDEBUG("usb_otgcore_open() success\n");
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
usb_otgcore_release(struct inode *inode, struct file *filp)
{
    struct otg_core_drv *otg_core = the_otg_core;
    
    PDEBUG("usb_otgcore_release() success\n");


    
    /* 使用中を解除 */
    clear_bit(USB_OTGCORE_ATOMIC_FD_LOCK, &otg_core->bitflags);
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static struct file_operations usb_otgcore_fops = 
{
    .owner  = THIS_MODULE,
    .open   = usb_otgcore_open,
    .release = usb_otgcore_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .unlocked_ioctl  = usb_otgcore_ioctl,
#else
    .ioctl  = usb_otgcore_ioctl,
#endif
};

struct cdev usb_otgcore_device;

/*-------------------------------------------------------------------------*/
static int otg_core_bind(struct usb_otg_control *otg_control)
{
    struct otg_core_drv *otg_core = the_otg_core;

    PDEBUG("%s call\n", __func__);
    
    /* otg_controlを保存 */
    otg_core->otg_control = otg_control;

    /* bind済みフラグをセット */
    set_bit(USB_OTGCORE_ATOMIC_BIND, &otg_core->bitflags);
    
    return 0;
}

static int otg_core_unbind(struct usb_otg_control *otg_control)
{
    struct otg_core_drv *otg_core = the_otg_core;
    
    PDEBUG("%s call\n", __func__);
    
    /* bind済みフラグをクリア */
    clear_bit(USB_OTGCORE_ATOMIC_BIND, &otg_core->bitflags);
    
    /* otg_controlを破棄 */
    otg_core->otg_control = NULL;
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
event_cid_exec(struct usb_otg_event_cid *pcid)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct m_otg_drv *tmp_otg_drv;
    struct usb_kevent_arg_otgcore_cid cid;
    unsigned long flags;
    int rc;
    // struct usb_otg_control_port_info port_info; <- 使わない

    if(otg_core->port_num == USB_OTGCORE_PORT_NOSET || otg_core->port_num < pcid->port ){
        PERR("event_cid_exec() param error\n");
        return -EINVAL;
    }
    switch(pcid->value){
      case 0:
        PDEBUG("  USB_OTG_EVENT_TYPE_CID: a-device port:%d\n", pcid->port);
        otg_core->port[pcid->port].cid = USB_OTGCORE_CID_A;
        cid.value = USB_OTGCORE_CID_A;
        break;
      
      case 1:
        PDEBUG("  USB_OTG_EVENT_TYPE_CID: b-device port:%d\n", pcid->port);
        otg_core->port[pcid->port].cid = USB_OTGCORE_CID_B;
        cid.value = USB_OTGCORE_CID_B;
        break;
      
      default:
        return -EINVAL;
    }
    
    /***** スピンロック *****/
    spin_lock_irqsave( &otg_core->lock, flags );
    
    /* リストからOTG Driverを取り出してnotifyを呼ぶ */
    list_for_each_entry( tmp_otg_drv, &otg_core->drv_list, list ){
        
        if( tmp_otg_drv->otg_drv->notify_with_param ){
            
            if ( cid.value == USB_OTGCORE_CID_A ){
                
                PDEBUG( "notify( USB_OTGCORE_EVT_CID_A ) to %p\n", tmp_otg_drv->otg_drv );
                tmp_otg_drv->otg_drv->notify_with_param( tmp_otg_drv->otg_drv, 
                                            USB_OTGCORE_EVT_CID_A, (int*)(&pcid->port) );
                
            }else if( cid.value == USB_OTGCORE_CID_B ){
                
                PDEBUG( "notify( USB_OTGCORE_EVT_CID_B ) to %p\n", tmp_otg_drv->otg_drv );
                tmp_otg_drv->otg_drv->notify_with_param( tmp_otg_drv->otg_drv, 
                                            USB_OTGCORE_EVT_CID_B, (int*)(&pcid->port) );
                
            }
            
        }
        
    }
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore( &otg_core->lock, flags );
    
    /* CIDがBに変化して かつ現在通信中のportだったら通知 */
    if(pcid->value == 1){
        
        /* 通信中のport取得 */
        // usb_otg_control_get_port_info(otg_core->otg_control, &port_info); <- 使わない
        
        if(the_port_info.current_port == pcid->port){
            
            /***** スピンロック *****/
            spin_lock_irqsave(&otg_core->lock, flags);
            
            /* リストからOTG Driverを取り出してnotifyを呼ぶ */
            list_for_each_entry(tmp_otg_drv, &otg_core->drv_list, list){
                if(tmp_otg_drv->otg_drv->notify){
                    PDEBUG("notify(USB_OTGCORE_CID_A_TO_B) to %p\n", tmp_otg_drv->otg_drv);
                    tmp_otg_drv->otg_drv->notify(tmp_otg_drv->otg_drv, USB_OTGCORE_CID_A_TO_B);
                }
            }
            
            /***** スピンロック解除 *****/
            spin_unlock_irqrestore(&otg_core->lock, flags);
            
        }else{
            PDEBUG("CID_A_TO_B but port no match!!\n");
        }
        
    }
    
    /* probeされていて、cb先が存在すれば eventにaddqueueする */
    rc = 0;
    cid.port = pcid->port;
    if(otg_core->otg_probe.hndl && otg_core->otg_probe.event.cid ){
        rc = usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                 otg_core->otg_probe.event.cid,
                                 otg_core->otg_probe.hndl,
                                 USB_KEVENT_ID_OTGCORE_CID,
                                 sizeof(cid),
                                 (void*)&cid );
    }
    
    return rc;
}

static int
event_vbus_exec(struct usb_otg_event_vbus *pvbus)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct usb_kevent_arg_otgcore_vbus vbus;
    int rc;
    int old_vbus = otg_core->port[pvbus->port].vbus;
    unsigned long flags;
    struct m_otg_drv *tmp_otg_drv;
    
    if(otg_core->port_num == USB_OTGCORE_PORT_NOSET || otg_core->port_num < pvbus->port ){
        PERR("event_vbus_exec() param error\n");
        return -EINVAL;
    }
    switch(pvbus->vbus_stat){
      case USB_OTG_VBUS_STAT_OFF:
        PDEBUG("  USB_OTG_EVENT_TYPE_VBUS: USB_OTG_VBUS_STAT_OFF port:%d\n", pvbus->port);
        otg_core->port[pvbus->port].vbus = USB_OTGCORE_VBUS_OFF;
        break;
        
      case USB_OTG_VBUS_STAT_LOW:
        PDEBUG("  USB_OTG_EVENT_TYPE_VBUS: USB_OTG_VBUS_STAT_LOW port:%d\n", pvbus->port);
        otg_core->port[pvbus->port].vbus = USB_OTGCORE_VBUS_OFF;
        break;
        
      case USB_OTG_VBUS_STAT_SESS:
        PDEBUG("  USB_OTG_EVENT_TYPE_VBUS: USB_OTG_VBUS_STAT_SESS port:%d\n", pvbus->port);
        otg_core->port[pvbus->port].vbus = USB_OTGCORE_VBUS_ON;
        break;
        
      case USB_OTG_VBUS_STAT_VALID:
        PDEBUG("  USB_OTG_EVENT_TYPE_VBUS: USB_OTG_VBUS_STAT_VALID port:%d\n", pvbus->port);
        otg_core->port[pvbus->port].vbus = USB_OTGCORE_VBUS_ON;
        break;
        
      default:
        return -EINVAL;
    }
    
    /***** スピンロック *****/
    spin_lock_irqsave( &otg_core->lock, flags );
    
    /* リストからOTG Driverを取り出してnotifyを呼ぶ */
    list_for_each_entry( tmp_otg_drv, &otg_core->drv_list, list ){
        
        if( tmp_otg_drv->otg_drv->notify_with_param ){
            
            if ( otg_core->port[pvbus->port].vbus == USB_OTGCORE_VBUS_ON ){
                
                PDEBUG( "notify( USB_OTGCORE_EVT_VBUS_ON ) to %p\n", tmp_otg_drv->otg_drv );
                tmp_otg_drv->otg_drv->notify_with_param( tmp_otg_drv->otg_drv, 
                                            USB_OTGCORE_EVT_VBUS_ON, (int*)(&pvbus->port) );
                
            }else if( otg_core->port[pvbus->port].vbus == USB_OTGCORE_VBUS_OFF ){
                
                PDEBUG( "notify( USB_OTGCORE_EVT_VBUS_OFF ) to %p\n", tmp_otg_drv->otg_drv );
                tmp_otg_drv->otg_drv->notify_with_param( tmp_otg_drv->otg_drv, 
                                            USB_OTGCORE_EVT_VBUS_OFF, (int*)(&pvbus->port) );
                
            }
            
        }
        
    }
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore( &otg_core->lock, flags );
    
    /* VBUSの状態が前回から変化していれば上位へ通知 */
    rc = 0;
    if(old_vbus != otg_core->port[pvbus->port].vbus){
        vbus.value = otg_core->port[pvbus->port].vbus;
        vbus.port  = pvbus->port;
        PDEBUG("AddQueue\n");
        if(otg_core->otg_probe.hndl && otg_core->otg_probe.event.vbus ){
            rc = usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                     otg_core->otg_probe.event.vbus,
                                     otg_core->otg_probe.hndl,
                                     USB_KEVENT_ID_OTGCORE_VBUS,
                                     sizeof(vbus),
                                     (void*)&vbus );
        }
    }
    
    return rc;
}

static int
event_vbus_error_exec(void)
{
    struct otg_core_drv *otg_core = the_otg_core;
    int rc;
    
    PDEBUG("  USB_OTG_EVENT_TYPE_VBUSERROR\n");
    
    /* 上位へ通知 */
    rc = 0;
    if(otg_core->otg_probe.hndl && otg_core->otg_probe.event.vbus_error ){
        rc = usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                 otg_core->otg_probe.event.vbus_error,
                                 otg_core->otg_probe.hndl,
                                 USB_KEVENT_ID_OTGCORE_VBUS_ERROR,
                                 0,
                                 NULL );
    }
    
    return rc;
}

static int
event_pullup_exec(struct usb_otg_event_con *con)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct m_otg_drv *tmp_otg_drv;
    struct usb_kevent_arg_otgcore_pullup pullup;
    unsigned long flags;
    int rc;
    unsigned char cid = otg_core->port[0].cid;
    unsigned char ctrl_speed = usb_otg_control_get_speed(otg_core->otg_control);
    
    switch(con->value){
      case 0:
        PDEBUG("  USB_OTG_EVENT_TYPE_CON: disconnect\n");
        if(ctrl_speed == USB_OTG_SPEED_FS && cid == USB_OTGCORE_CID_A){
			PDEBUG("USB_OTGCORE_PULLUP_OFF Do not notify\n");
			PDEBUG("Now changing EHCI -> OHCI\n");
        	return 0;
        }
        pullup.value = USB_OTGCORE_PULLUP_OFF;
        break;
        
      case 1:
        PDEBUG("  USB_OTG_EVENT_TYPE_CON: connect\n");
        if(ctrl_speed == USB_OTG_SPEED_FS){
			PDEBUG("USB_OTGCORE_PULLUP_ON Do not notify\n");
			PDEBUG("Now changing EHCI -> OHCI\n");
			return 0;
		}
		pullup.value = USB_OTGCORE_PULLUP_ON;
        break;
      
      default:
        return -EINVAL;
    }
    
    /* probeされていて、cb先が存在すれば eventにaddqueueする */
    rc = 0;

    /* 暫定対策 複数ポート対応未 ポート固定の通知とする */
    if( the_port_info.current_port ){
        
        pullup.port = USB_OTGCORE_PORT1;
        
    }else{
        
        pullup.port = USB_OTGCORE_PORT0;
        
    }

    /* PullUP OFF時にはOTG Driverへ通知 */
    if(con->value == 0){
        /***** スピンロック *****/
        spin_lock_irqsave(&otg_core->lock, flags);
        
        /* リストからOTG Driverを取り出してnotifyを呼ぶ */
        list_for_each_entry(tmp_otg_drv, &otg_core->drv_list, list){
            if(tmp_otg_drv->otg_drv->notify){
                PDEBUG("notify(USB_OTGCORE_B_DISCONNECT) to %p\n", tmp_otg_drv->otg_drv);
                tmp_otg_drv->otg_drv->notify(tmp_otg_drv->otg_drv, USB_OTGCORE_B_DISCONNECT);
            }
        }
        
        /***** スピンロック解除 *****/
        spin_unlock_irqrestore(&otg_core->lock, flags);
    }
    
    if(otg_core->otg_probe.hndl && otg_core->otg_probe.event.pullup ){
        rc = usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                 otg_core->otg_probe.event.pullup,
                                 otg_core->otg_probe.hndl,
                                 USB_KEVENT_ID_OTGCORE_PULLUP,
                                 sizeof(pullup),
                                 (void*)&pullup );
    }
    
    return rc;
}

static int
event_receive_srp_exec(void)
{
    struct otg_core_drv *otg_core = the_otg_core;
    int rc;
    
    PDEBUG("  USB_OTG_EVENT_TYPE_SRP\n");
    
    /* 上位へ通知 */
    rc = 0;
    if(otg_core->otg_probe.hndl && otg_core->otg_probe.event.receive_srp ){
        rc = usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                 otg_core->otg_probe.event.receive_srp,
                                 otg_core->otg_probe.hndl,
                                 USB_KEVENT_ID_OTGCORE_RECEIVE_SRP,
                                 0,
                                 NULL );
    }
    
    return rc;
}

static int
event_rchost_end_exec(unsigned int status)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct usb_kevent_arg_otgcore_rchost rchost;
    unsigned long flags;
    int mode;
    int rc;
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    switch(status){
      case USB_OTG_SUCCESS:
        PDEBUG("  USB_OTG_EVENT_TYPE_CON: Success\n");
        
        /* enable_rchost()済みを解除 */
        clear_bit(USB_OTGCORE_ATOMIC_ENABLE_RCHOST, &otg_core->bitflags);
        
        rchost.result = USB_OTGCORE_SUCCESS;
        
        /* RCHostでなかったらすでにmodeが切り替わっているので
           何もしないで終了 */
        if(mode != USB_OTG_CONTROL_RCHOST){
            return 0;
        }
        
        break;
        
      case USB_OTG_FAIL_T_B_ASE0_BRST:
        PDEBUG("  USB_OTG_EVENT_TYPE_CON: TimeOut t_b_ase0_brst\n");
        
        rchost.result = USB_OTGCORE_FAIL;
        
        /* Gadgetでなかったらすでにmodeが切り替わっているので
           何もしないで終了 */
        if(mode != USB_OTG_CONTROL_GADGET){
            return 0;
        }
        
        break;
      
      default:
        return -EINVAL;
    }
    
    /* probeされていて、cb先が存在すれば eventにaddqueueする */
    rc = 0;
    if(otg_core->otg_probe.hndl && otg_core->otg_probe.event.rchost ){
        rc = usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                 otg_core->otg_probe.event.rchost,
                                 otg_core->otg_probe.hndl,
                                 USB_KEVENT_ID_OTGCORE_RCHOST,
                                 sizeof(rchost),
                                 (void*)&rchost );
    }
    
    return rc;
}

static int
event_rcgadget_end_exec(unsigned int status)
{
    struct otg_core_drv *otg_core = the_otg_core;
    struct usb_kevent_arg_otgcore_rcgadget rcgadget;
    unsigned long flags;
    int mode;
    int rc;
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    switch(status){
      case USB_OTG_SUCCESS:
        PDEBUG("  USB_OTG_EVENT_TYPE_RCGADGET: Success\n");
        
        rcgadget.result = USB_OTGCORE_SUCCESS;
        
        break;
      
      case USB_OTG_FAIL_T_A_AIDL_BDIS:
        PDEBUG("  USB_OTG_EVENT_TYPE_RCGADGET: TimeOut t_a_aidl_bdis\n");
        
        rcgadget.result = USB_OTGCORE_FAIL;
        
        break;
      
      default:
        return -EINVAL;
        break;
    }
    
    /* RCGadgetでなかったらすでにmodeが切り替わっているので
       何もしないで終了 */
    if(mode != USB_OTG_CONTROL_RCGADGET){
        return 0;
    }
    
    /* probeされていて、cb先が存在すれば eventにaddqueueする */
    rc = 0;
    if(otg_core->otg_probe.hndl && otg_core->otg_probe.event.rcgadget ){
        rc = usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                                 otg_core->otg_probe.event.rcgadget,
                                 otg_core->otg_probe.hndl,
                                 USB_KEVENT_ID_OTGCORE_RCGADGET,
                                 sizeof(rcgadget),
                                 (void*)&rcgadget );
    }
    
    return rc;
}

static int otg_core_notify(union usb_otg_event *otg_event)
{
    int rc;
    struct otg_core_drv *otg_core = the_otg_core;
    
    /* 引数が異常でないことを確認 */
    if(!otg_event){
        return -EINVAL;
    }
    
    PDEBUG("event notify:\n");
    switch(otg_event->type){
      
      /* CID 変化検出 */
      case USB_OTG_EVENT_TYPE_CID:
        rc = event_cid_exec(&(otg_event->cid));
        break;
        
      /* VBUS 変化検出 */
      case USB_OTG_EVENT_TYPE_VBUS:
        if( otg_core->port_num < 2 ){
            rc = event_vbus_exec(&(otg_event->vbus));
        }else{
            // Nothing to do.
        }
        break;
        
      /* VBUS Error 検出 */
      case USB_OTG_EVENT_TYPE_VBUSERROR:
        rc = event_vbus_error_exec();
        break;
        
      /* PullUP 変化検出 */
      case USB_OTG_EVENT_TYPE_CON:
        rc = event_pullup_exec(&(otg_event->con));
        break;
      
      /* SRP 検出 */
      case USB_OTG_EVENT_TYPE_SRP:
        rc = event_receive_srp_exec();
        break;
      
      /* RollChange to Host 終了 */
      case USB_OTG_EVENT_TYPE_RCHOST:
        rc = event_rchost_end_exec(otg_event->rchost.status);
        break;
      
      /* RollChange to Gadget 終了 */
      case USB_OTG_EVENT_TYPE_RCGADGET:
        rc = event_rcgadget_end_exec(otg_event->rcgadget.status);
        break;
      
      default:
        return -EINVAL;
    }
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int __init
otg_core_alloc(void)
{
    struct otg_core_drv *otg_core;
    
    /* g_core 用の領域を取得 */
    otg_core = kmalloc(sizeof *otg_core, GFP_KERNEL);
    if(!otg_core) return -ENOMEM;
    
    /* g_core を初期化 */
    memset(otg_core, 0, sizeof *otg_core);
    
    /* TestMode していない状態にセット */
    otg_core->test_mode = USB_OTGCORE_TEST_MODE_NORMAL;
    
    /* HS Enableにセット */
    otg_core->hs_disable = USB_OTGCORE_HS_ENABLE;

    /* 想定される最大port数分のport情報用領域を確保 初期化はopen時に行う*/
    otg_core->port = kmalloc((sizeof(struct usb_otgcore_port) * USB_OTGCORE_PORT_MAX), GFP_ATOMIC);
    if(!otg_core->port){
        kfree(otg_core);
        PWARN("otg_core_alloc() malloc err\n");
        return -ENOMEM;
    }
    
    the_otg_core = otg_core;
    
    PDEBUG("the_otg_core : 0x%lx\n", (unsigned long)the_otg_core);
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static void
otg_core_free(struct otg_core_drv *otg_core)
{
    kfree(otg_core->port);
    kfree(otg_core);
}

/*-------------------------------------------------------------------------*/
static int
proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    struct otg_core_drv *otg_core = the_otg_core;
    // struct usb_otg_control_port_info port_info; <- 使わない
    unsigned long flags;
    int len = 0, write_size;
    int mode;
    unsigned char cid = 0;
    unsigned char vbus = 0;
    enum usb_otg_control_speed bus_speed = 0;
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /* USB port数を取得 */
    // usb_otg_control_get_port_info(otg_core->otg_control,&port_info); <- 使わない
    
    if(otg_core->port_num == USB_OTGCORE_PORT_NOSET  || otg_core->port_num < the_port_info.current_port){
        return -EINVAL;
    }

    if(mode != USB_OTG_CONTROL_STOP){
        /* CIDを取得 */
        cid = otg_core->port[the_port_info.current_port].cid;
        
        /* VBUSを取得 */
        vbus = otg_core->port[the_port_info.current_port].vbus;
        
        /* OTG Controller から BusSpeedを取得 */
        bus_speed = usb_otg_control_get_speed(otg_core->otg_control);
    }
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    /* modeがSTOP以外なら、BusSpeedをOTGCoreで定義している値に変換 */
    if(mode != USB_OTG_CONTROL_STOP){
        switch(bus_speed){
          case USB_OTG_SPEED_LS:
            bus_speed = USB_OTGCORE_SPEED_LS;
            break;
            
          case USB_OTG_SPEED_FS:
            bus_speed = USB_OTGCORE_SPEED_FS;
            break;
            
          case USB_OTG_SPEED_HS:
            bus_speed = USB_OTGCORE_SPEED_HS;
            break;
          
          default:
            bus_speed = 0;
        }
    }
    
    /* modeを追記 */
    write_size = snprintf(page+len, count, "%02d ", mode);
    if(write_size > count){
        len += count;
        goto exit;
    }
    len += write_size;
    count -= write_size;
    
    /* CIDを追記 */
    write_size = snprintf(page+len, count, "%02d ", cid);
    if(write_size > count){
        len += count;
        goto exit;
    }
    len += write_size;
    count -= write_size;
    
    /* VBUSを追記 */
    write_size = snprintf(page+len, count, "%02d ", vbus);
    if(write_size > count){
        len += count;
        goto exit;
    }
    len += write_size;
    count -= write_size;
    
    /* BusSpeedを追記 */
    write_size = snprintf(page+len, count, "%02d ", bus_speed);
    if(write_size > count){
        len += count;
        goto exit;
    }
    len += write_size;
    
exit:
    *eof = 1;
    
    return len;
}

#if defined(CONFIG_ARCH_CXD90014BASED)
  #define USB_OTGCORE_CXD90014_USB_ID_EXT_ADDR     ( 0xF2920010 )
  #define USB_OTGCORE_CXD90014_USB_VBUS_STATE_MASK ( 0x00000001 )
  #define USB_OTGCORE_CXD90014_USB_ID_STATE_MASK   ( 0x00000002 )
#endif
/*-------------------------------------------------------------------------*/
static void usb_otgcore_notify_vbusoff_cidb( void )
{
    
    struct usb_otg_event_vbus st_vbus;
    struct usb_otg_event_cid st_cid;
    struct otg_core_drv *otg_core = the_otg_core;
    
    PDEBUG("%s call\n", __func__);
    
    // VBUS OFF for Port 0
    st_vbus.type = USB_OTG_EVENT_TYPE_VBUS;
    st_vbus.port = 0;
    st_vbus.vbus_stat = USB_OTG_VBUS_STAT_OFF;
    event_vbus_exec( &st_vbus );
    
    // CID_B for Port 0
    st_cid.type = USB_OTG_EVENT_TYPE_CID;
    st_cid.port = 0;
    st_cid.value = 1;
    event_cid_exec( &st_cid );
    
    if( otg_core->port_num == 2 ){
        
        // VBUS OFF for Port 1
        st_vbus.type = USB_OTG_EVENT_TYPE_VBUS;
        st_vbus.port = 1;
        st_vbus.vbus_stat = USB_OTG_VBUS_STAT_OFF;
        event_vbus_exec( &st_vbus );
        
        // CID_B for Port 1
        st_cid.type = USB_OTG_EVENT_TYPE_CID;
        st_cid.port = 1;
        st_cid.value = 1;
        event_cid_exec( &st_cid );
        
    }
    
    return;
    
}
/*-------------------------------------------------------------------------*/
static void usb_otgcore_notify_current_vbus_state( void )
{
    
    UDIF_U32 reg_value = 0;
    struct usb_otg_event_vbus st_vbus;
    struct usb_otg_event_vbus st_vbus_stda;
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned int data = 0;
    
    PDEBUG("%s call\n", __func__);
    
    /* Port 0 */
    st_vbus.type = USB_OTG_EVENT_TYPE_VBUS;
    st_vbus.port = 0;
    
    if( otg_core->port_num == 2 ){
        
        gpio_get_data_bit( the_port_desc.port[0].vbus.gpio_port, the_port_desc.port[0].vbus.gpio_bit, &data );
        
        if( data ){
            
            // VBUS OFF
            st_vbus.vbus_stat = USB_OTG_VBUS_STAT_OFF;
            
        }else{
            
            // VBUS ON
            st_vbus.vbus_stat = USB_OTG_VBUS_STAT_VALID;
            
        }
        
    }else{
        
#if defined(CONFIG_ARCH_CXD90014BASED)
        reg_value = udif_ioread32( USB_OTGCORE_CXD90014_USB_ID_EXT_ADDR );
        PDEBUG( "Current ID_EXT value is 0x%08X \n", reg_value );
        
        if( ( reg_value & USB_OTGCORE_CXD90014_USB_VBUS_STATE_MASK ) == 0x00000000 ){
            
            // VBUS ON
            st_vbus.vbus_stat = USB_OTG_VBUS_STAT_VALID;
            
        }else{
            
            // VBUS OFF
            st_vbus.vbus_stat = USB_OTG_VBUS_STAT_OFF;
            
        }
#endif
        
    }
    
    event_vbus_exec( &st_vbus );
    
    /* Port 1 */
    st_vbus_stda.type = USB_OTG_EVENT_TYPE_VBUS;
    st_vbus_stda.port = 1;
    
    if( otg_core->port_num == 2 ){
        
        gpio_get_data_bit( the_port_desc.port[1].vbus.gpio_port, the_port_desc.port[1].vbus.gpio_bit, &data );
        PDEBUG( "XVBUS_DET_STD_A_PORT status : %d \n", data );
        
        if( data ){
            
            // VBUS OFF
            st_vbus_stda.vbus_stat = USB_OTG_VBUS_STAT_OFF;
            
        }else{
            
            // VBUS ON
            st_vbus_stda.vbus_stat = USB_OTG_VBUS_STAT_VALID;
            
        }
        
        event_vbus_exec( &st_vbus_stda );
        
    }
    
    return;
    
}

/*-------------------------------------------------------------------------*/
static void usb_otgcore_notify_current_id_state( void )
{
    
    UDIF_U32 reg_value = 0;
    struct usb_otg_event_cid st_cid;
    struct usb_otg_event_cid st_cid_stda;
    struct otg_core_drv *otg_core = the_otg_core;
    
    PDEBUG("%s call\n", __func__);
    
    /* Port 0 */
    st_cid.type = USB_OTG_EVENT_TYPE_CID;
    st_cid.port = 0;
    
#if defined(CONFIG_ARCH_CXD90014BASED)
    reg_value = udif_ioread32( USB_OTGCORE_CXD90014_USB_ID_EXT_ADDR );
    PDEBUG( "Current ID_EXT value is 0x%08X \n", reg_value );
    
    if( ( reg_value & USB_OTGCORE_CXD90014_USB_ID_STATE_MASK ) == 0x00000000 ){
        
        // CID_A
        st_cid.value = 0;
        
    }else{
        
        // CID_B
        st_cid.value = 1;
        
    }
    
    event_cid_exec( &st_cid );
#endif
    
    /* Port 1 */
    st_cid_stda.type = USB_OTG_EVENT_TYPE_CID;
    st_cid_stda.port = 1;
    st_cid.value = 1; // CID_B Fixed
    
    if( otg_core->port_num == 2 ){
        
        event_cid_exec( &st_cid );
        
    }
    
    return;
    
}

static irqreturn_t usb_otgcore_vbus_IrqHandler( int irq, void* p_dev_id )
{
    
    struct usb_otg_event_vbus st_vbus;
    unsigned int data = 0;
    
    st_vbus.type = USB_OTG_EVENT_TYPE_VBUS;
    st_vbus.port = 0;
    
    PDEBUG("%s call\n", __func__);
    
    gpio_get_data_bit( the_port_desc.port[0].vbus.gpio_port, the_port_desc.port[0].vbus.gpio_bit, &data );
    
    if( data ){
        
        // VBUS OFF
        st_vbus.vbus_stat = USB_OTG_VBUS_STAT_OFF;
        
    }else{
        
        // VBUS ON
        st_vbus.vbus_stat = USB_OTG_VBUS_STAT_VALID;
        
    }
    
    event_vbus_exec( &st_vbus );
    
    return IRQ_HANDLED;
    
}

#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
/*-------------------------------------------------------------------------*/
static irqreturn_t usb_otgcore_id_IrqHandler( int irq, void* p_dev_id )
{
    
    UDIF_U32 reg_value = 0;
    struct usb_otg_event_cid st_cid;
    
    st_cid.type = USB_OTG_EVENT_TYPE_CID;
    st_cid.port = 0;
    
    PDEBUG("%s call\n", __func__);
    
#if defined(CONFIG_ARCH_CXD90014BASED)
    reg_value = udif_ioread32( USB_OTGCORE_CXD90014_USB_ID_EXT_ADDR );
    PDEBUG( "Current ID_EXT value is 0x%08X \n", reg_value );
    
    if( ( reg_value & USB_OTGCORE_CXD90014_USB_ID_STATE_MASK ) == 0x00000000 ){
        
        // CID A
        st_cid.value = 0;
        
    }else{
        
        // CID B
        st_cid.value = 1;
        
    }
    
    event_cid_exec( &st_cid );
#endif
    
    return IRQ_HANDLED;
    
}
#endif

static irqreturn_t usb_otgcore_vbus_stda_IrqHandler( int irq, void* p_dev_id )
{
    
    struct usb_otg_event_vbus st_vbus;
    unsigned int data = 0;
    
    st_vbus.type = USB_OTG_EVENT_TYPE_VBUS;
    st_vbus.port = 1;
    
    PDEBUG("%s call\n", __func__);
    
    gpio_get_data_bit( the_port_desc.port[1].vbus.gpio_port, the_port_desc.port[1].vbus.gpio_bit, &data );
    
    if( data ){
        
        // VBUS OFF
        st_vbus.vbus_stat = USB_OTG_VBUS_STAT_OFF;
        
    }else{
        
        // VBUS ON
        st_vbus.vbus_stat = USB_OTG_VBUS_STAT_VALID;
        
    }
    
    event_vbus_exec( &st_vbus );
    
    return IRQ_HANDLED;
    
}

static void usb_otgcore_request_vbusid_irq( void )
{
    
    int err = 0;
//    unsigned int data = 0;
//    UDIF_U32 reg_value = 0;
    
//    struct usb_otg_event_vbus st_vbus;
//    struct usb_otg_event_vbus st_vbus_stda;
//    struct usb_otg_event_cid st_cid_stda;
    struct otg_core_drv *otg_core = the_otg_core;
//    struct usb_otg_event_cid st_cid;
    
    PDEBUG("%s call\n", __func__);
    
    /* Notify VBUS/ID State before request_irq() */
#if 0
    if( otg_core->port_num == 2 ){
        /* --- Port 0 --- */
        // VBUS
        st_vbus.type = USB_OTG_EVENT_TYPE_VBUS;
        st_vbus.port = 0;
        
        gpio_get_data_bit( the_port_desc.port[0].vbus.gpio_port, the_port_desc.port[0].vbus.gpio_bit, &data );
        PDEBUG( "Port 0 status : %d \n", data );
        
        if( data ){
            
            // VBUS OFF
            st_vbus.vbus_stat = USB_OTG_VBUS_STAT_OFF;
            
        }else{
            
            // VBUS ON
            st_vbus.vbus_stat = USB_OTG_VBUS_STAT_VALID;
            
        }
        
        event_vbus_exec( &st_vbus );
        
        // CID
        st_cid.type = USB_OTG_EVENT_TYPE_CID;
        st_cid.port = 0;
        
        reg_value = udif_ioread32( USB_OTGCORE_CXD90014_USB_ID_EXT_ADDR );
        PDEBUG( "Current ID_EXT value is 0x%08X \n", reg_value );
        
        if( ( reg_value & USB_OTGCORE_CXD90014_USB_ID_STATE_MASK ) == 0x00000000 ){
            
            // CID A
            st_cid.value = 0;
            
        }else{
            
            // CID B
            st_cid.value = 1;
            
        }
        
        event_cid_exec( &st_cid );
        
        /* --- Port 1 --- */
        // VBUS
        st_vbus_stda.type = USB_OTG_EVENT_TYPE_VBUS;
        st_vbus_stda.port = 1;
        
        gpio_get_data_bit( the_port_desc.port[1].vbus.gpio_port, the_port_desc.port[1].vbus.gpio_bit, &data );
        PDEBUG( "Port 1 status : %d \n", data );
        
        if( data ){
            
            // VBUS OFF
            st_vbus_stda.vbus_stat = USB_OTG_VBUS_STAT_OFF;
            
        }else{
            
            // VBUS ON
            st_vbus_stda.vbus_stat = USB_OTG_VBUS_STAT_VALID;
            
        }
        
        event_vbus_exec( &st_vbus_stda );
        
        // CID
        st_cid_stda.type = USB_OTG_EVENT_TYPE_CID;
        st_cid_stda.port = 1;
        st_cid_stda.value = 1;
        
        event_cid_exec( &st_cid_stda );
        
    }
#endif
    
    /* request_irq() */
    
    if( otg_core->port_num == 2 ){
#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
        // Request irq for ID Pin change event.
        err = request_irq( IRQ_USB_ID, usb_otgcore_id_IrqHandler, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "usbid", NULL );
        
        if( err < 0 ){
            
            PERR("request_irq(usbid) error. \n");
            goto SUB_RET;
        }
#endif
        
        /* --- Port 0 --- */
        vbus_irq_no = gpiopin_to_irq( the_port_desc.port[0].vbus.gpio_port, the_port_desc.port[0].vbus.gpio_bit );
        PDEBUG( "Port 0 irq_no : %d \n", vbus_irq_no );
        
        if( vbus_irq_no == -1 ){
            
            PERR("gpiopin_to_irq(usbvbus) error. %d \n", vbus_irq_no);
#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
            free_irq( IRQ_USB_ID, NULL );
#endif
            goto SUB_RET;
            
        }
        
        err = request_irq( vbus_irq_no, usb_otgcore_vbus_IrqHandler, IRQF_TRIGGER_RISING, "usbvbus", NULL );
        
        if( err < 0 ){
            
            PERR("request_irq(usbvbus) error. %d \n", err);
#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
            free_irq( IRQ_USB_ID, NULL );
#endif
            goto SUB_RET;
            
        }
        
        /* --- Port 1 --- */
        vbus_stda_irq_no = gpiopin_to_irq( the_port_desc.port[1].vbus.gpio_port, the_port_desc.port[1].vbus.gpio_bit );
        PDEBUG( "Port 1 irq_no : %d \n", vbus_stda_irq_no );
        
        if( vbus_stda_irq_no == -1 ){
            
            PERR("gpiopin_to_irq(usbvbus_stda) error : %d \n", vbus_stda_irq_no);
#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
            free_irq( IRQ_USB_ID, NULL );
#endif
            free_irq( vbus_irq_no, NULL );
            vbus_irq_no = -1;
            goto SUB_RET;
            
        }
        
        err = request_irq( vbus_stda_irq_no, usb_otgcore_vbus_stda_IrqHandler, IRQF_TRIGGER_RISING, "usbvbus_stda", NULL );
        
        if( err < 0 ){
            
            PERR("request_irq(usbvbus_stda) error : %d \n", err);
#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
            free_irq( IRQ_USB_ID, NULL );
#endif
            free_irq( vbus_irq_no, NULL );
            vbus_irq_no = -1;
            goto SUB_RET;
            
        }
        
    }
    

    
SUB_RET:
    return;
    
}

static void usb_otgcore_free_vbusid_irq( void )
{
    
    struct otg_core_drv *otg_core = the_otg_core;
    
    PDEBUG("%s call\n", __func__);
    
    if( otg_core->port_num == 2 ){
#if defined( USB_OTGCORE_VBUS_ID_IRQ_BY_ME )
        free_irq( IRQ_USB_ID, NULL );
#endif
        
        /* Port 0 */
        if( vbus_irq_no != -1 ){
            
            free_irq( vbus_irq_no, NULL );
            vbus_irq_no = -1;
            
        }
        
        /* Port 1 */
        if( vbus_stda_irq_no != -1 ){
            
            free_irq( vbus_stda_irq_no, NULL );
            vbus_stda_irq_no = -1;
            
        }
        
    }
    
    return;
    
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int __init usb_otgcore_module_init(void)
{
    UDIF_DEVNODE *node;
    UDIF_DEVNO devno;
    struct otg_core_drv *otg_core;
    int res;
    
    PINFO("USB OTGCore driver ver " USBOTGCORE_VERSION "\n");
    
    /* otg_core_drv用の領域を確保 */
    res = otg_core_alloc();
    if(res != 0){
        res = -EINVAL;
        goto exit;
    }
    
    /* OTG Core Driver の実体を取得 */
    otg_core = the_otg_core;
    
    /* FunctionDriverList を初期化する */
    INIT_LIST_HEAD(&otg_core->drv_list);
    
    /* セマフォを初期化 */
    udif_mutex_init(&otg_core->sem);
    
    /* spin_lock用変数を初期化 */
    spin_lock_init(&otg_core->lock);
    
    /* Controller へ regist する */
    res = usb_otg_control_register_core(&otg_core_driver);
    if(res != 0){
        PDEBUG("usb_otg_control_register_core() failed\n");
        
        /* otg_core構造体を開放 */
        otg_core_free(otg_core);
        res = -EINVAL;
        goto exit;
    }
    
    // CharacterDeviceの登録
    node = udif_device_node(UDIF_NODE_USB_OCORE);
    devno = UDIF_MKDEV(node->major, node->first_minor);
    
    res = register_chrdev_region(devno, node->nr_minor, node->name);
    if(res != 0){
        PDEBUG("register_chrdev_region() failed\n");
        
        /* Controller からunregistする */
        usb_otg_control_unregister_core(&otg_core_driver);
        
        /* otg_core構造体を開放 */
        otg_core_free(otg_core);
        goto exit;
    }
    
    cdev_init(&usb_otgcore_device, &usb_otgcore_fops);
    
    res = cdev_add(&usb_otgcore_device, devno, node->nr_minor);
    if(res != 0){
        PDEBUG("cdev_add() failed\n");
        
        /* Controller からunregistする */
        usb_otg_control_unregister_core(&otg_core_driver);
        
        /* otg_core構造体を開放 */
        otg_core_free(otg_core);
        
        unregister_chrdev_region(devno, node->nr_minor);
        goto exit;
    }
    
    /* procfile登録 */
    create_proc_read_entry(OTGCORE_PROCFILE, 0, NULL, proc_read, NULL);
    res = 0;
    
    the_port_info.nr = 0;
    the_port_info.current_port = 0;
    
    PDEBUG("usb_otgcore_module_init() success\n");
    
exit:
    return res;
}

/*-------------------------------------------------------------------------*/
static void __exit usb_otgcore_module_exit(void)
{
    UDIF_DEVNODE *node;
    UDIF_DEVNO devno;
    struct otg_core_drv *otg_core = the_otg_core;
    unsigned long flags;
    int mode;
    int res;
    
    PDEBUG("%s call\n", __func__);
    
    /* procファイル削除 */
    remove_proc_entry(OTGCORE_PROCFILE, NULL);
    
    /***** スピンロック *****/
    spin_lock_irqsave(&otg_core->lock, flags);
    
    /* 現在のモードを取得 */
    mode = usb_otg_control_get_mode(otg_core->otg_control);
    
    /***** スピンロック解除 *****/
    spin_unlock_irqrestore(&otg_core->lock, flags);
    
    
    /* stop状態でなかったらstopへ遷移 */
    if(mode != USB_OTG_CONTROL_STOP){
        res = transition_to_idle();
        PDEBUG("transition_to_idle() rc: %d\n", res);
        if(res == 0){
            res = transition_to_stop();
            PDEBUG("transition_to_stop() rc: %d\n", res);
        }
    }
    
    // CharacterDeviceの登録削除
    node = udif_device_node(UDIF_NODE_USB_OCORE);
    devno = UDIF_MKDEV(node->major, node->first_minor);
    
    cdev_del(&usb_otgcore_device);
    unregister_chrdev_region(devno, node->nr_minor);
    
    /* Controller から unregist する */
    res = usb_otg_control_unregister_core(&otg_core_driver);
    if(res != 0){
        PDEBUG("usb_otg_control_unregister_core() failed\n");
    }
    
    /* otg_core構造体を開放 */
    otg_core_free(otg_core);
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*=============================================================================
 * Export symbols
 *===========================================================================*/
EXPORT_SYMBOL(usb_otgcore_register_driver);
EXPORT_SYMBOL(usb_otgcore_unregister_driver);
EXPORT_SYMBOL(usb_otgcore_get_hs_disable);
EXPORT_SYMBOL(usb_otgcore_gadget_set_feature);
EXPORT_SYMBOL(usb_otgcore_gadget_suspend);
EXPORT_SYMBOL(usb_otgcore_gadget_disconnect);
EXPORT_SYMBOL(usb_otgcore_get_line_state);

module_exit(usb_otgcore_module_exit);
