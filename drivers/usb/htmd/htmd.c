/*
 * htmd.c
 * 
 * Copyright 2008,2013 Sony Corporation
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#if defined(CONFIG_ARCH_CXD4132BASED)

#include <linux/usb/scd/scd_hub.h>
/* SCD HUB Command */
#define HTMD_DEVREQ_H2D_V_O         SCD_DEVREQ_H2D_V_O
/* #define HTMD_PORT_FEAT_EP_NO_DMA    SCD_PORT_FEAT_EP_NO_DMA  SCD�ł͍폜 */
#define HTMD_PORT_FEAT_TEST_SE0     SCD_PORT_FEAT_TEST_SE0
#define HTMD_PORT_FEAT_TEST_J       SCD_PORT_FEAT_TEST_J
#define HTMD_PORT_FEAT_TEST_K       SCD_PORT_FEAT_TEST_K
#define HTMD_PORT_FEAT_TEST_PACKET  SCD_PORT_FEAT_TEST_PACKET
#define HTMD_PORT_FEAT_TEST_SETUP   SCD_PORT_FEAT_TEST_SETUP
#define HTMD_PORT_FEAT_TEST_IN      SCD_PORT_FEAT_TEST_IN

#elif defined(CONFIG_ARCH_CXD4120)
#include <linux/usb/mcd/usb_mcd_hub.h>
/* SCD HUB Command */
#define HTMD_DEVREQ_H2D_V_O         MCD_DEVREQ_H2D_V_O
/* #define HTMD_PORT_FEAT_EP_NO_DMA    MCD_PORT_FEAT_EP_NO_DMA */
#define HTMD_PORT_FEAT_TEST_SE0     MCD_PORT_FEAT_TEST_SE0
#define HTMD_PORT_FEAT_TEST_J       MCD_PORT_FEAT_TEST_J
#define HTMD_PORT_FEAT_TEST_K       MCD_PORT_FEAT_TEST_K
#define HTMD_PORT_FEAT_TEST_PACKET  MCD_PORT_FEAT_TEST_PACKET
#define HTMD_PORT_FEAT_TEST_SETUP   MCD_PORT_FEAT_TEST_SETUP
#define HTMD_PORT_FEAT_TEST_IN      MCD_PORT_FEAT_TEST_IN

#else

#include <linux/usb/f_usb/f_usb20ho_hub.h>
#define HTMD_DEVREQ_H2D_V_O         F_USB20HO_DEVREQ_H2D_V_O
/* #define HTMD_PORT_FEAT_EP_NO_DMA    F_USB20HO_PORT_FEAT_EP_NO_DMA */
#define HTMD_PORT_FEAT_TEST_SE0     F_USB20HO_PORT_FEAT_TEST_SE0
#define HTMD_PORT_FEAT_TEST_J       F_USB20HO_PORT_FEAT_TEST_J
#define HTMD_PORT_FEAT_TEST_K       F_USB20HO_PORT_FEAT_TEST_K
#define HTMD_PORT_FEAT_TEST_PACKET  F_USB20HO_PORT_FEAT_TEST_PACKET
#define HTMD_PORT_FEAT_TEST_SETUP   F_USB20HO_PORT_FEAT_TEST_SETUP
#define HTMD_PORT_FEAT_TEST_IN      F_USB20HO_PORT_FEAT_TEST_IN
#define HTMD_PORT_FEAT_FORCE_ENABLE F_USB20HO_PORT_FEAT_TEST_FORCE_ENABLE
#define HTMD_PORT_FEAT_TEST_SUSPEND F_USB20HO_PORT_FEAT_TEST_SUSPEND

#endif

/*-------------------------------------------------------------------------*/
#define DRIVER_AUTHOR       "Sony Corporation"
#define DRIVER_NAME         "htmd"

#define HTD_VID             0x1a0a

#define HTD_PID_SE0         0x0101
#define HTD_PID_J           0x0102
#define HTD_PID_K           0x0103
#define HTD_PID_PKT         0x0104
#define HTD_PID_SUS_RES     0x0106
#define HTD_PID_STEP        0x0107
#define HTD_PID_STEP_DATA   0x0108

static struct usb_device_id htd_ids [] = {
    { USB_DEVICE(HTD_VID,  HTD_PID_SE0 ) },
    { USB_DEVICE(HTD_VID,  HTD_PID_J   ) },
    { USB_DEVICE(HTD_VID,  HTD_PID_K   ) },
    { USB_DEVICE(HTD_VID,  HTD_PID_PKT ) },
    { USB_DEVICE(HTD_VID,  HTD_PID_SUS_RES   ) },
    { USB_DEVICE(HTD_VID,  HTD_PID_STEP      ) },
    { USB_DEVICE(HTD_VID,  HTD_PID_STEP_DATA ) },
    { },
};

/*-------------------------------------------------------------------------*/
#define ATOMIC_FLAG_GOTO_EXIT   0

#define WAIT_TIME               15

/* for Debug */
#if CONFIG_USB_HOST_TEST_MODE_LOGGING > 0
#define PDEBUG(fmt,args...)     printk(DRIVER_NAME ": " fmt , ## args)
#else
#define PDEBUG(fmt,args...)     do { } while (0)
#endif

#if CONFIG_USB_HOST_TEST_MODE_LOGGING > 1
#define PVERBOSE                PDEBUG
#else
#define PVERBOSE(fmt,args...)   do { } while (0)
#endif

#if CONFIG_USB_HOST_TEST_MODE_LOGGING != 0
#define PINFO(fmt,args...)      printk(DRIVER_NAME ": " fmt , ## args)
#else
#define PINFO(fmt,args...)      do { } while (0)
#endif

#define PERR(fmt,args...)       printk(KERN_ERR DRIVER_NAME ": " fmt , ## args)

/*-------------------------------------------------------------------------*/
static int send_vmsg(struct usb_device *rhdev, u8 mode);
static int send_vcmsg(struct usb_device *rhdev, u8 mode);
static int send_cmsg(struct usb_device *rhdev, u8 mode);
static int time_wait(void);
static int htd_probe(struct usb_interface *intf, const struct usb_device_id *id);
static void htd_disconnect(struct usb_interface *intf);
static int __init htd_init(void);
static void __exit htd_exit(void);

/*-------------------------------------------------------------------------*/
unsigned long       atomic_bitflags;

/*-------------------------------------------------------------------------*/
static int send_vmsg(struct usb_device *rhdev, u8 mode)
{
    return usb_control_msg(rhdev, usb_rcvctrlpipe(rhdev, 0),
                           USB_REQ_SET_FEATURE,
                           HTMD_DEVREQ_H2D_V_O,
                           mode,
                           1, /* wIndex, '1' is port# */
                           NULL,
                           0,
                           USB_CTRL_SET_TIMEOUT);
}

static int send_vcmsg(struct usb_device *rhdev, u8 mode)
{
    return usb_control_msg(rhdev, usb_rcvctrlpipe(rhdev, 0),
                           USB_REQ_CLEAR_FEATURE,
                           HTMD_DEVREQ_H2D_V_O,
                           mode,
                           1, /* wIndex, '1' is port# */
                           NULL,
                           0,
                           USB_CTRL_SET_TIMEOUT);
}

static int send_cmsg(struct usb_device *rhdev, u8 mode)
{
    return usb_control_msg(rhdev, usb_rcvctrlpipe(rhdev, 0),
                           USB_REQ_SET_FEATURE,
                           0x23, /* Host2Dev HubClass Other */
                           21, /* USB_PORT_FEAT_TEST @core/hub.h*/
                           (mode << 8) | 1, /* wIndex, '1' is port# */
                           NULL,
                           0,
                           USB_CTRL_SET_TIMEOUT);
}

static int time_wait(void)
{
    unsigned int i;
    int res = 0;
    
    PINFO("Sleep %d sec\n", WAIT_TIME);
    
    for(i = 0; i < WAIT_TIME; i++){
        if(test_bit(ATOMIC_FLAG_GOTO_EXIT, &atomic_bitflags)){
            PERR("Test Mode Canceled\n");
            res = -1;
            goto SUB_RET;
        }
        ssleep(1);
    }
    
SUB_RET:
    return res;
}

static int htd_probe(struct usb_interface *intf,
             const struct usb_device_id *id)
{
    int res;
    struct usb_device *udev = interface_to_usbdev(intf);
    struct usb_device *rhdev = udev->bus->root_hub;
    
    usb_set_intfdata (intf, NULL);
    
    PINFO("VID: 0x%04x\n", id->idVendor);
    PINFO("PID: 0x%04x\n", id->idProduct);
    
    if(test_bit(ATOMIC_FLAG_GOTO_EXIT, &atomic_bitflags)){
        res = -1;
        goto SUB_RET;
    }
    
    switch(id->idProduct){
    case HTD_PID_SE0:
        PINFO("Test Mode Start: SE0\n");
        res = send_vmsg(rhdev, HTMD_PORT_FEAT_TEST_SE0);
        // send_cmsg(rhdev, 3);
        break;
        
    case HTD_PID_J:
        PINFO("Test Mode Start: J\n");
        res = send_vmsg(rhdev, HTMD_PORT_FEAT_TEST_J);
        // send_cmsg(rhdev, 1);
        break;
        
    case HTD_PID_K:
        PINFO("Test Mode Start: K\n");
        res = send_vmsg(rhdev, HTMD_PORT_FEAT_TEST_K);
        // send_cmsg(rhdev, 2);
        break;
        
    case HTD_PID_PKT:
        PINFO("Test Mode Start: PACKET\n");
        res = send_vmsg(rhdev, HTMD_PORT_FEAT_TEST_PACKET);
        // send_cmsg(rhdev, 4);
        break;
        
    case HTD_PID_SUS_RES:
        PINFO("Test Mode Start: Chirp/Suspend/Resume\n");
        
        /* Wait for Bus Suspend */
        if(time_wait()){
            res = -1;
            goto SUB_RET;
        }
        
        PINFO("Bus Suspend\n");
        res = send_vmsg(rhdev, HTMD_PORT_FEAT_TEST_SUSPEND);
        
        if(res){
            PINFO("Bus Suspend failed.\n");
            goto SUB_RET;
        }
        
        /* Wait for Bus Resume */
        if(time_wait()){
            res = -1;
            goto SUB_RET;
        }
        
        PINFO("Bus Resume\n");
        res = send_vcmsg(rhdev, HTMD_PORT_FEAT_TEST_SUSPEND);
        
        if(res){
            PINFO("Bus Resume failed.\n");
            goto SUB_RET;
        }
        
        /* There is an ehci driver restriction. 
           "Whoever resumes must GetPortStatus to complete it!!" */
        res = usb_control_msg(rhdev, usb_rcvctrlpipe(rhdev, 0),
                              USB_REQ_GET_STATUS,
                              0xa3, /* GetPortStatus */
                              0, /* wValue ignore */
                              1, /* wIndex = port */
                              NULL,
                              0,
                              USB_CTRL_SET_TIMEOUT);
        if(res){
            PINFO("GetPortStatus failed.\n");
            goto SUB_RET;
        }
        
        PINFO("Test Mode End\n");
        break;
        
    case HTD_PID_STEP:
        PINFO("Test Mode Start: Step Get Device Descriptor\n");
        res = send_cmsg(rhdev, 5);
        if(res){
            goto SUB_RET;
        }
        
        if(time_wait()){
            res = -1;
            goto SUB_RET;
        }
        
        PINFO("HTMD_PORT_FEAT_TEST_SETUP\n");
        res = send_vmsg(rhdev, HTMD_PORT_FEAT_TEST_SETUP);
        
        PINFO("Test Mode End\n");
        
        break;
        
    case HTD_PID_STEP_DATA:
        PINFO("Test Mode Start: Step Get Device Descriptor Data\n");
        res = send_cmsg(rhdev, 5);
        if(res){
            goto SUB_RET;
        }
        
        if(time_wait()){
            res = -1;
            goto SUB_RET;
        }
        
        PINFO("HTMD_PORT_FEAT_TEST_SETUP\n");
        res = send_vmsg(rhdev, HTMD_PORT_FEAT_TEST_SETUP);
        if(res){
            goto SUB_RET;
        }
        
        if(time_wait()){
            res = -1;
            goto SUB_RET;
        }
        
        PINFO("HTMD_PORT_FEAT_TEST_IN\n");
        res = send_vmsg(rhdev, HTMD_PORT_FEAT_TEST_IN);
        
        PINFO("Test Mode End\n");
        
        break;
        
    default:
        PERR("PID:0x%04x is not support\n", id->idProduct);
        res = -1;
        break;
    }
    
SUB_RET:
    if(res != 0){
        PERR("Error: Test Mode\n");
    }
    
    return res;
}

static void htd_disconnect(struct usb_interface *intf)
{
    struct usb_device *udev = interface_to_usbdev(intf);
    
    usb_set_intfdata(intf, NULL);
    
    usb_put_dev(usb_get_dev(udev));
    
    dev_info(&intf->dev, "USB Host Testmode Device now disconnected\n");
}

static struct usb_driver htd_driver = {
    .name =         "htmd",
    .probe =        htd_probe,
    .disconnect =   htd_disconnect,
    .id_table =     htd_ids,
};

static int __init htd_init(void)
{
    int rc;
    
    clear_bit(ATOMIC_FLAG_GOTO_EXIT, &atomic_bitflags);
    
    rc = usb_register(&htd_driver);
    
    return rc;
}
module_init(htd_init);

static void __exit htd_exit(void)
{
    set_bit(ATOMIC_FLAG_GOTO_EXIT, &atomic_bitflags);
    
    usb_deregister(&htd_driver);
}
module_exit(htd_exit);

MODULE_DESCRIPTION("htmd - USB Host Testmode Driver");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
