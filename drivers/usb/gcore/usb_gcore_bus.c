/*
 * usb_gcore_bus.c
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

#include <linux/usb/gcore/usb_event.h>
#include <linux/usb/gcore/usb_gadgetcore.h>
#include <linux/usb/gcore/usb_otgcore.h>

#include "usb_gadgetcore_cfg.h"
#include "usb_gadgetcore_pvt.h"

/*=============================================================================
 *
 * Main function body
 *
 *===========================================================================*/
/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * bus_suspend
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    BusがHostによってsuspend状態になった時の処理を行う
 * 入力:    g_core  : GadgetCoreへのポインタ
 * 出力:    
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
void
bus_suspend(struct g_core_drv *g_core)
{
    struct g_func_drv *tmp_func_drv;
    
    /* OTG Coreへsuspendが来たことを通知し、RCHostの開始かを確認する */
    if(usb_otgcore_gadget_suspend() == USB_OTGCORE_RES_START_RCHOST){
        /* RCHostの開始のsuspendであったならば、FuncDrv、上位への
           通知は行わない */
        PDEBUG("resul: USB_OTGCORE_RES_START_RCHOST\n");
        return ;
    }
    
    /* 登録されているFuncDrv全てに対して、suspend通知を送る */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->suspend){
            PVERBOSE(" suspend() to %s\n", 
                     tmp_func_drv->func_drv->function ? 
                     tmp_func_drv->func_drv->function : "");
            tmp_func_drv->func_drv->suspend(tmp_func_drv->func_drv);
        }
    }
    
    /* 上位へsuspend通知を発行する */
    PDEBUG("AddQueue(suspend)\n");
    if(g_core->g_probe.hndl && g_core->g_probe.event.suspend){
        usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                            g_core->g_probe.event.suspend,
                            g_core->g_probe.hndl,
                            USB_KEVENT_ID_GADGETCORE_SUSPEND,
                            0,
                            NULL );
    }
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * bus_resume
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    BusがHostによってresume状態になった時の処理を行う
 * 入力:    g_core  : GadgetCoreへのポインタ
 * 出力:    
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
void
bus_resume(struct g_core_drv *g_core)
{
    struct g_func_drv *tmp_func_drv;
    
    /* 登録されているFuncDrv全てに対して、resume通知を送る */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->resume){
            PVERBOSE(" resume() to %s\n", 
                     tmp_func_drv->func_drv->function ? 
                     tmp_func_drv->func_drv->function : "");
            tmp_func_drv->func_drv->resume(tmp_func_drv->func_drv);
        }
    }
    
    /* 上位へresume通知を発行する */
    PDEBUG("AddQueue(resume)\n");
    if(g_core->g_probe.hndl && g_core->g_probe.event.resume){
        usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                            g_core->g_probe.event.resume,
                            g_core->g_probe.hndl,
                            USB_KEVENT_ID_GADGETCORE_RESUME,
                            0,
                            NULL );
    }
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * bus_disconnect
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    BusがHostから切断またはBusResetされた時の処理を行う
 * 入力:    g_core  : GadgetCoreへのポインタ
 * 出力:    
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
void
bus_disconnect(struct g_core_drv *g_core)
{
    
    struct g_func_drv *tmp_func_drv;
    
    /* OTG Coreへdisconnectを通知 */
    usb_otgcore_gadget_disconnect();
    
    /* Setupモジュールへdisconnectを通知 */
    setup_disconnect(g_core);
    
    /* 登録されているFuncDrv全てに対して、BusReset通知を送る */
    list_for_each_entry(tmp_func_drv, &g_core->func_list, list){
        if(tmp_func_drv->func_drv->busreset){
            PVERBOSE(" BusReset() to %s\n", 
                     tmp_func_drv->func_drv->function ? 
                     tmp_func_drv->func_drv->function : "");
            tmp_func_drv->func_drv->busreset(tmp_func_drv->func_drv);
        }
    }
    
    /* 上位へBusReset通知を発行する */
    PDEBUG("AddQueue(bus_reset)\n");
    if(g_core->g_probe.hndl && g_core->g_probe.event.bus_reset){
        usb_event_add_queue(USB_EVENT_PRI_NORMAL,
                            g_core->g_probe.event.bus_reset,
                            g_core->g_probe.hndl,
                            USB_KEVENT_ID_GADGETCORE_BUS_RESET,
                            0,
                            NULL );
    }
}
