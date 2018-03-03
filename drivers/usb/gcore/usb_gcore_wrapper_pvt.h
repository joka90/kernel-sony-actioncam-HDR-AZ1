/*
 * usb_gcore_wrapper_pvt.h
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
#ifndef ___USB_GCORE_WRAPPER_PVT_H
#define ___USB_GCORE_WRAPPER_PVT_H

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

#include <linux/usb/gcore/usb_event.h>
#include <linux/usb/gcore/usb_gadgetcore.h>
#include <linux/usb/gcore/usb_otgcore.h>

#include "usb_gadgetcore_pvt.h"
#include "usb_gcore_wrapper_cfg.h"

/*-----------------------------------------------------------------------------
 * Debug
 *---------------------------------------------------------------------------*/
//--------
// for DEBUGs
#define DBG_LVL 0        // 0:Err 1:+Wrn 2:+Prn 3:+Inf 4:+Dmp
//#define DBG_OUT_MEM    // printk the alloced/freed mem addr
//--------


#if DBG_LVL >= 0
# include <linux/string.h>
# ifndef DBG_PREFIX
#  define DBG_PREFIX "GCORE_DEFAULT"
# endif
# define CLR_ON  "\x1b[1;36m"
# define CLR_OFF "\x1b[0m"
# include <linux/sched.h>
static inline void _sleep( int time ){
    int timeout = jiffies+time*HZ;
    while( jiffies < timeout ){
        schedule();
    }
}
# define SLEEP(x) _sleep(x)
#endif

#if DBG_LVL == 0
# define DEBUG_PRINTK(fmt, ... ) 
# define DEBUG_ERR(fmt, ... )    
# define DEBUG_WARN(fmt, ... )   
# define DEBUG_PRINT(fmt, ... )  
# define DEBUG_INFO(fmt, ... )   
# define DEBUG_DUMP(x,y)
#endif
#if DBG_LVL == 1
# define DEBUG_PRINTK(fmt, ... ) printk("[DBG:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_ERR(fmt, ... )    printk("[ERR:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_WARN(fmt, ... )   printk("[WRN:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_PRINT(fmt, ... )  
# define DEBUG_INFO(fmt, ... )   
# define DEBUG_DUMP(x,y)         
#endif
#if DBG_LVL == 2
# define DEBUG_PRINTK(fmt, ... ) printk("[DBG:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_ERR(fmt, ... )    printk("[ERR:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_WARN(fmt, ... )   printk("[WRN:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_PRINT(fmt, ... )  printk("[PRN:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_INFO(fmt, ... )   
# define DEBUG_DUMP(x,y)         
#endif
#if DBG_LVL == 3
# define DEBUG_PRINTK(fmt, ... ) printk("[DBG:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_ERR(fmt, ... )    printk("[ERR:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_WARN(fmt, ... )   printk("[WRN:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_PRINT(fmt, ... )  printk("[PRN:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_INFO(fmt, ... )   printk("[INF:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_DUMP(x,y)         
#endif
#if DBG_LVL == 4
# define DEBUG_PRINTK(fmt, ... ) printk("[DBG:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_ERR(fmt, ... )    printk("[ERR:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_WARN(fmt, ... )   printk("[WRN:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_PRINT(fmt, ... )  printk("[PRN:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
# define DEBUG_INFO(fmt, ... )   printk("[INF:" CLR_ON DBG_PREFIX CLR_OFF ": %04d]" fmt "\n", __LINE__ ,## __VA_ARGS__)
static void _debug_dump( u8* buff,  int len ){
    int i;
    char    str[16 + 1];        /* 16Byteごとに表示 */
    printk( "---- dump data in maincnt( %p,%x ) -----------\n", buff, len );
    for (i = 0; i < len; i++) {
        if( (i%0x8000<0x0010) ||
            (i<0x0020) ||
            (i<0x0030) ||
            (i<0x0040) ||
            (i<0x0050) ||
            (i<0x0060) ||
            //(i<0x0070) ||
            ((i/0x0010)>(len/0x0010-2)) )
          { //間引き

              if ((i % 16) == 0) {
                  printk("%04x : ", (i / 16) * 16);
              }

              /* 文字の代入   */
              if ((*(buff + i) < ' ') || (*(buff + i) > '~')) {
                  str[i % 16] = ' ';              /* EscapeはSPに変換 */
              } else {
                  str[i % 16] = *(buff + i);
              }

              /* バイナリ表示 */
              printk("%02x ", *(buff + i));

              /* 文字列表示   */
              if ((i % 16) == 15) {
                  str[16] = '\0';
                  printk(": %s\n", str);
              }
              else if (i == (len-1)){
                  str[(i % 16) + 1] = '\0';
                  printk(": %s\n", str);
              }
          }
    }
    printk( "---------------------------------------------\n" );
}
# define DEBUG_DUMP(x,y) _debug_dump(x,y)
#endif

#ifdef DBG_OUT_MEM
# include <linux/string.h>
# ifndef DBG_PREFIX
#  define DBG_PREFIX "GCORE_DEFAULT"
# endif
# define DEBUG_NEW(x)  printk("[MEM:" CLR_ON DBG_PREFIX CLR_OFF "] %s(%04d),%s,+,%p\n", __FUNCTION__, __LINE__ , #x, x );
# define DEBUG_FREE(x) printk("[MEM:" CLR_ON DBG_PREFIX CLR_OFF "] %s(%04d),%s,-,%p\n", __FUNCTION__, __LINE__ , #x, x );
#else
# define DEBUG_NEW(x)  
# define DEBUG_FREE(x) 
#endif

/*-----------------------------------------------------------------------------
 * struct declaration
 *---------------------------------------------------------------------------*/
#define USBGADGETCORE_NAME_WRAPPER "usb_gadgetcore_wrapper"

/*--- GadgetCoreWrapper lock info -----------------------------*/
typedef struct {
    spinlock_t lock; // for Spin-lock
} GCW_LOCK_INFO;

/*--- GadgetCoreWrapper States --------------------------------*/
typedef enum {
    GCW_STATE_NONE=0,
    GCW_STATE_INITIALIZED,
    GCW_STATE_SINGLE,
    GCW_STATE_DUAL_IDLE,
    GCW_STATE_DUAL_STARTED,
    GCW_STATE_DUAL_CONVERTING,
    GCW_STATE_DUAL_FIXED_SIC,
    GCW_STATE_DUAL_FIXED,
    //
    GCW_STATE_LAST,
} GCW_STATE;

/*--- GadgetCoreWrapper Info --------------------------------*/
#define GCW_MAX_EP_BUFF_LEN 512

struct gcw_info{
    GCW_BOOL  enable_dual_mode;
    GCW_STATE state;
    u8        buff[GCW_MAX_EP_BUFF_LEN];
    int       buff_len;
};

/*--- OS Descriptor --------------------------------*/
struct usb_os_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 qwSignature[14];
    __u8 bMS_VendorCode;
    __u8 bPad;
} __attribute__ ((packed));

/*--- Extended Configuration Descriptor --------------------------------*/
struct usb_extended_configuration_descriptor {
    /* header section */
    __u32 dwLength;
    __u16 bcdVersion;
    __u16 wIndex;
    __u8 bCount;
    __u8 bRESERVED[7];
    /* function section */
    __u8 bFirstInterfaceNumber;
    __u8 bInterfaceCount;
    __u8 compatibleID[8];
    __u8 subCompatibleID[8];
    __u8 bRESERVED2[6];
} __attribute__ ((packed));


/*-----------------------------------------------------------------------------
 * Macro declaration
 *---------------------------------------------------------------------------*/
#define GET_EP_ADR(x) ((struct g_ep_info*)(x)->_ep->driver_data)->ep_adr

/*-----------------------------------------------------------------------------
 * Function prototypes
 *---------------------------------------------------------------------------*/
// for functions for all.
GCW_BOOL  change_gcw_state( GCW_STATE new_state );
GCW_STATE get_gcw_state( void );
char* get_gcw_state_str( GCW_STATE state );
int  usb_gcw_start(GCW_BOOL enable_flag);
int  usb_gcw_stop(void);

// for functions for usb_gcore_setup.c
int usb_gcw_setup_setconfig( u8 new_config );
int usb_gcw_setup_setinterface( u8 new_config );

// for functions for usb_gcore_wrapper.c
int does_match_epadr( struct g_core_ep *gep, u8 ep_adr );
int is_BULKOUT_EP( struct g_core_ep *gep );
int is_BULKOUT_MTP( u8* buff, int size );

// for functions for usb_gcore_main.c
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
int  usb_gcw_probe_driver(struct usb_gadget_driver* g_core_driver, int (*bind)(struct usb_gadget *));
#else
int  usb_gcw_register_driver(struct usb_gadget_driver* g_core_driver);
#endif
int  usb_gcw_unregister_driver(struct usb_gadget_driver* g_core_driver);
int  gcw_bind(struct usb_gadget*);
void gcw_unbind(struct usb_gadget*);
int  gcw_setup(struct usb_gadget*, const struct usb_ctrlrequest*);
void gcw_disconnect(struct usb_gadget*);
void gcw_suspend(struct usb_gadget*);
void gcw_resume(struct usb_gadget*);

struct g_func_drv *findFuncDrv_fromIfClasses(u8, u8, u8);
struct usb_gadget_ep_list stopFuncDrv_fromIfClasses(u8, u8, u8);
int startFuncDrv_fromIfClasses(u8, u8, u8, struct usb_gadget_ep_list, unsigned char);

// for functions for usb_gcore_ep.c
int  usb_gcw_register_ep_ops(struct usb_ep_ops*);


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
struct usb_request *gcw_alloc_request (struct usb_ep *ep, gfp_t gfp_flags);
#else
struct usb_request *gcw_alloc_request (struct usb_ep *ep, int gfp_flags);
#endif
void gcw_free_request (struct usb_ep *ep, struct usb_request *req);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
void *gcw_alloc_buffer( struct usb_ep *ep, unsigned bytes, dma_addr_t *dma, gfp_t gfp_flags );
#else
void *gcw_alloc_buffer( struct usb_ep *ep, unsigned bytes, dma_addr_t *dma, int gfp_flags );
#endif
void gcw_free_buffer( struct usb_ep *ep, void *buf, dma_addr_t dma, unsigned bytes );
#endif

void gcw_complete(struct usb_ep *_ep, struct usb_request *_req);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
int gcw_queue(struct usb_ep *ep, struct usb_request *req, gfp_t gfp_flags);
#else
int gcw_queue(struct usb_ep *ep, struct usb_request *req, int gfp_flags);
#endif

int gcw_dequeue(struct usb_ep *ep, struct usb_request *req);
void gcw_fifo_flush(struct usb_ep *ep);
int gcw_fifo_status(struct usb_ep *ep);
int gcw_set_halt(struct usb_ep *ep, int value);


#endif //#define ___USB_GCORE_WRAPPER_PVT_H
