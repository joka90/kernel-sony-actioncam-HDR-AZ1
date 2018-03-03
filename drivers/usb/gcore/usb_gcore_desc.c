/*
 * usb_gcore_desc.c
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

#include "usb_gadgetcore_cfg.h"
#include "usb_gadgetcore_pvt.h"

#ifdef DBG_PREFIX
# undef  DBG_PREFIX
# define DBG_PREFIX "GCORE_DESC"
#else
# define DBG_PREFIX "GCORE_DESC"
#endif
#include "usb_gcore_wrapper_cfg.h"
#include "usb_gcore_wrapper_pvt.h"

/*-----------------------------------------------------------------------------
 * Function prototype declaration
 *---------------------------------------------------------------------------*/
static int save_langids(usb_gadget_desc_table*);
static int save_string_desc(usb_gadget_langid_table*);
static int save_strings(usb_gadget_string_desc*);
static int overwrite_iserial(usb_gadget_desc_table*, __u8, void*);
static int save_config_desc(usb_gadget_speed_desc*);
static int save_report_desc(usb_gadget_report_desc*);
static int save_otg_desc(usb_gadget_config_desc*);
static int save_ia_desc(usb_gadget_config_desc*);
static int save_interface_desc(usb_gadget_config_desc*);
static int save_interface_table(usb_gadget_interface_desc*);
static int save_pep_list(usb_gadget_if_table*);
static int save_class_if_desc(usb_gadget_if_table*);
static int save_class_ep_desc(usb_gadget_if_table_element*);

static int free_langids(usb_gadget_desc_table*);
static int free_string_desc(usb_gadget_langid_table*);
static int free_strings(usb_gadget_string_desc*);
static int free_config_desc(usb_gadget_speed_desc*);
static int free_report_desc(usb_gadget_report_desc*);
static int free_otg_desc(usb_gadget_config_desc*);
static int free_ia_desc(usb_gadget_config_desc*);
static int free_interface_desc(usb_gadget_config_desc*);
static int free_interface_table(usb_gadget_interface_desc*);
static int free_class_if_desc(usb_gadget_if_table*);
static int free_pep_list(usb_gadget_if_table*);
static int free_class_ep_desc(usb_gadget_if_table_element*);

static void dump_config_desc(usb_gadget_config_desc*);
static void dump_otg_desc(usb_gadget_otg_desc*);
static void dump_ia_desc(usb_gadget_ia_desc*);
static void dump_interface_desc(usb_gadget_interface_desc*);
static void dump_if_tables(usb_gadget_if_table*);
static void dump_pep_list(usb_gadget_if_table_element*);
static void dump_langid_table(usb_gadget_langid_table*);
static void dump_string_desc(usb_gadget_string_desc*);

/*=============================================================================
 *
 * Main function body
 *
 *===========================================================================*/
/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * get_config_desc_from_config_num
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    現在のBusSpeedで、引数で指定されたConfig番号を持つConfigDesc
 *          を取得する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          config_num : Config番号
 * 出力:    !0   : 成功(ConfigDescへのポインタ)
 *          NULL : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
usb_gadget_config_desc*
get_config_desc_from_config_num(struct g_core_drv *g_core, u8 config_num)
{
    usb_gadget_speed_desc *sp_desc;
    u8 i;
    
    /* 現在のspeedからSpeedDescを取得 */
    if(g_core->gadget->speed == USB_SPEED_HIGH){
        sp_desc = &g_core->desc_tbl->hs;
    }else{
        sp_desc = &g_core->desc_tbl->lsfs;
    }
    
    /* 指定のconfig値を持つConfigDescriptorを探す */
    for(i = 0; i < sp_desc->uc_num_config; i++){
        if(sp_desc->configurations[i].uc_configuration_value == config_num){
            return &sp_desc->configurations[i];
        }
    }
    
    /* 見つからなかった */
    return NULL;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * get_interface_desc
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    現在のBusSpeedで、引数で指定されたConfig,Interface
 *          番号を持つinterface_descを取得する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          config_num    : Config番号
 *          interface_num : Interface番号
 * 出力:    !0   : 成功(interface_descへのポインタ)
 *          NULL : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
usb_gadget_interface_desc*
get_interface_desc(struct g_core_drv *g_core, u8 config_num, u8 interface_num)
{
    usb_gadget_config_desc *cfg_desc;
    u8 i;
    
    /* 指定のConfigからConfigDescを取得 */
    cfg_desc = get_config_desc_from_config_num(g_core, config_num);
    if(cfg_desc == NULL){
        return NULL;
    }
    
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        if(cfg_desc->interfaces[i].uc_if_number == interface_num){
            return &cfg_desc->interfaces[i];
        }
    }
    
    return NULL;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * get_if_table_desc
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    現在のBusSpeedで、引数で指定されたConfig,Interface,Alternate
 *          番号を持つif_table_descを取得する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          config_num    : Config番号
 *          interface_num : Interface番号
 *          alt_num       : Alternate番号
 * 出力:    !0   : 成功(if_table_descへのポインタ)
 *          NULL : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
usb_gadget_if_table*
get_if_table_desc(struct g_core_drv *g_core, u8 config_num, u8 interface_num, u8 alt_num)
{
    usb_gadget_interface_desc *in_desc;
    u8 i;
    
    /* 指定のConfig、InterfaceからInterfaceDescを取得 */
    in_desc = get_interface_desc(g_core, config_num, interface_num);
    if(in_desc == NULL){
        return NULL;
    }
    
    for(i = 0; i < in_desc->uc_num_if_tables; i++){
        if(in_desc->if_tables[i].uc_setting_number == alt_num){
            return &in_desc->if_tables[i];
        }
    }
    
    return NULL;
}

/* 現在設定されているConfig番号のConfigDescriptorを返す */
usb_gadget_config_desc*
get_current_config_desc(struct g_core_drv *g_core)
{
    return get_config_desc_from_config_num(g_core, g_core->set_config);
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * make_alt_num_list
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    現在のConfigのInterface分のAlternate管理領域を確保し、
 *          初期化する
 * 入力:    g_core     : GadgetCoreへのポインタ
 * 出力:    0以上 : 成功
 *          0未満 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
u8 make_alt_num_list(struct g_core_drv *g_core)
{
    usb_gadget_config_desc *cfg_desc;
    struct alt_num *tmp;
    u8 i;
    
    PDEBUG("make_alt_num_list()\n");
    
    /* 現在のConfigDescriptorを取得 */
    cfg_desc = get_current_config_desc(g_core);
    
    /* Alternate番号保存領域を確保 */
    tmp = (struct alt_num*)kmalloc(sizeof(struct alt_num) * cfg_desc->uc_num_interfaces,
                                   GFP_ATOMIC);
    if(!tmp){
        PDEBUG("   --->Error kmalloc()\n");
        return -ENOMEM;
    }
    
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        tmp[i].interface_num = cfg_desc->interfaces[i].uc_if_number;
        tmp[i].alternate_num = 0;
    }
    g_core->alt_nums = tmp;
    PDEBUG("   --->OK alloc %d\n", cfg_desc->uc_num_interfaces);
    
    return cfg_desc->uc_num_interfaces;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * set_alt_num
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    Alternate管理領域内の指定Interface番号のInterfaceの
 *          Alternate番号を指定のAlternate番号に変更する
 *          初期化する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          in_num     : Interface番号
 *          alt_num    : 新たに設定するAlternate番号
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
int set_alt_num(struct g_core_drv *g_core, u8 in_num, u8 alt_num)
{
    usb_gadget_config_desc *cfg_desc;
    struct alt_num *tmp;
    u8 i;
    
    tmp = g_core->alt_nums;
    if(!tmp){
        return -EINVAL;
    }
    
    /* 現在のConfigDescriptorを取得 */
    cfg_desc = get_current_config_desc(g_core);
    
    /* Alt管理領域から指定のInterface番号を探す */
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        if(tmp[i].interface_num == in_num){
            /* InterfaceのAlt番号を指定のAlt番号に変更 */
            tmp[i].alternate_num = alt_num;
            return 0;
        }
    }
    
    return -EINVAL;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * get_alt_num
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    Alternate管理領域内の指定Interface番号のInterfaceの
 *          Alternate番号取得する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          in_num     : Interface番号
 *          *alt_num   : 取得したAlternate番号の保存先
 * 出力:     0 : 成功
 *          !0 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
int get_alt_num(struct g_core_drv *g_core, u8 in_num, u8 *alt_num)
{
    usb_gadget_config_desc *cfg_desc;
    struct alt_num *tmp;
    u8 i;
    
    tmp = g_core->alt_nums;
    if(!tmp){
        return -EINVAL;
    }
    
    /* 現在のConfigDescriptorを取得 */
    cfg_desc = get_current_config_desc(g_core);
    
    /* Alt管理領域から指定のInterface番号を探す */
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        if(tmp[i].interface_num == in_num){
            /* InterfaceのAlt番号を指定のAlt番号に変更 */
            *alt_num = tmp[i].alternate_num;
            return 0;
        }
    }
    
    return -EINVAL;
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * free_alt_num_list
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    Alternate管理領域を開放する
 * 入力:    g_core     : GadgetCoreへのポインタ
 * 出力:    0以上 : 成功
 *          0未満 : 失敗
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
void free_alt_num_list(struct g_core_drv *g_core)
{
    PDEBUG("kfree(g_core->alt_nums)\n");
    if(g_core->alt_nums){
        PDEBUG("    --->OK\n");
        kfree(g_core->alt_nums);
        g_core->alt_nums = NULL;
    }
}

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * is_hnp_capable
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*
 * 概要:    現在のConfigのConfigDescriptorでHNPが許可されているかを返す
 *          現在のConfig番号が0の時には、DescriptorTable内のDevice情報で
 *          HNPが許可されているか確認する
 * 入力:    g_core     : GadgetCoreへのポインタ
 *          config_num : Config番号
 * 出力:    !0 : HNPに対応
 *          0  : HNPに非対応
 * 
 * <EOS>                                *
 *━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
int is_hnp_capable(struct g_core_drv *g_core)
{
    usb_gadget_speed_desc *sp_desc;
    usb_gadget_config_desc *cfg_desc;
    u8 set_config;
    int res;
    
    /* 現在のConfig番号を取得(本当は、usb_gcore_setup.c から取得するようにする) */
    set_config = g_core->set_config;
    
    /* Configured StateかDefault,Address Stateかで処理を分ける */
    if(g_core->set_config != 0){
        /* Configured State */
        cfg_desc = get_config_desc_from_config_num(g_core, set_config);
        
        /* ConfigDescが取得できたか確認 */
        if(!cfg_desc){
            res = 0;
            goto exit;
        }
        
        /* OTG Descがあるか確認 */
        if(!cfg_desc->otg){
            res = 0;
            goto exit;
        }
        
        /* OTG Descのattributesを見て、HNPに対応しているか確認 */
        if((cfg_desc->otg->uc_attributes & USB_OTG_HNP) == 0){
            res = 0;
            goto exit;
        }
        res = 1;
        
    }else{
        /* Default,Address State */
        
        /* 現在のspeedからSpeedDescを取得 */
        if(g_core->gadget->speed == USB_SPEED_HIGH){
            sp_desc = &g_core->desc_tbl->hs;
        }else{
            sp_desc = &g_core->desc_tbl->lsfs;
        }
        
        /* Speed DescのOTGattributesを見て、HNPに対応しているか確認 */
        if((sp_desc->uc_otg_attributes & USB_OTG_HNP) == 0){
            res = 0;
            goto exit;
        }
        res = 1;
        
    }
    
exit:
    return res;
}

/*=============================================================================
 * Descriptor Generate
 *===========================================================================*/
int make_device_desc(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf)
{
    int rc;
    struct usb_device_descriptor dev_desc;
    usb_gadget_speed_desc *sp_desc;
    usb_gadget_desc_table *desc_tbl;
    
    desc_tbl = g_core->desc_tbl;
    
    if(g_core->gadget->speed == USB_SPEED_HIGH){
        PDEBUG("=================== HS =================== \n");
        sp_desc = &desc_tbl->hs;
    }else{
        PDEBUG("=================== FS =================== \n");
        sp_desc = &desc_tbl->lsfs;
    }
    
    dev_desc.bLength = sizeof dev_desc;
    dev_desc.bDescriptorType = USB_DT_DEVICE;
    
    dev_desc.bcdUSB = cpu_to_le16(sp_desc->us_bcd_usb);
    dev_desc.bDeviceClass = sp_desc->uc_class;
    dev_desc.bDeviceSubClass = sp_desc->uc_sub_class;
    dev_desc.bDeviceProtocol = sp_desc->uc_protocol;
    dev_desc.bMaxPacketSize0 = g_core->ep0->maxpacket;
    dev_desc.idVendor  = cpu_to_le16(desc_tbl->us_id_vendor);
    dev_desc.idProduct = cpu_to_le16(desc_tbl->us_id_product);
    dev_desc.bcdDevice = cpu_to_le16(desc_tbl->us_bcd_device);
    dev_desc.iManufacturer = desc_tbl->uc_manufacturer_str;
    dev_desc.iProduct = desc_tbl->uc_product_str;
    dev_desc.iSerialNumber = desc_tbl->uc_serial_number_str;
    dev_desc.bNumConfigurations = sp_desc->uc_num_config;
    
    rc = min(ctrl->wLength, (u16)sizeof(dev_desc));
    memcpy(buf, &dev_desc, rc);
    
    return rc;
}

int make_hid_desc(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf)
{
    
    int rc = 0;
    u8 i = 0;
    
    const u8 hid_class_num = 0x03;
    
    usb_gadget_desc_table *desc_tbl    = NULL;
    usb_gadget_speed_desc *sp_desc     = NULL;
    usb_gadget_config_desc *cfg_desc   = NULL;
    usb_gadget_interface_desc *in_desc = NULL;
    usb_gadget_if_table *if_tbl        = NULL;
    
    /* Get Descriptor Table */
    desc_tbl = g_core->desc_tbl;
    
    /* Get Speed Descriptor */
    if( g_core->gadget->speed == USB_SPEED_HIGH ){
        
        PDEBUG("=================== HS =================== \n");
        sp_desc = &desc_tbl->hs;
        
    }else{
        
        PDEBUG("=================== FS =================== \n");
        sp_desc = &desc_tbl->lsfs;
        
    }
    if( sp_desc == NULL ) return -EINVAL;
    
    /* Find Current Config */
    for( i = 0; i < sp_desc->uc_num_config; i++ ){
        
        if( sp_desc->configurations[i].uc_configuration_value == g_core->set_config ){
            
            cfg_desc = &sp_desc->configurations[i];
            break;
            
        }
        
    }
    if( cfg_desc == NULL ) return -EINVAL; /* Could not find... -> NOT Configured State. */
    
    /* Find Desired Interface Descriptor */
    for( i = 0; i < cfg_desc->uc_num_interfaces; i++ ){
        
        if( cfg_desc->interfaces[i].uc_if_number == (u8)(ctrl->wIndex & 0x00ff) ){
            
            in_desc = &cfg_desc->interfaces[i];
            break;
            
        }
        
    }
    if( in_desc == NULL ) return -EINVAL; /* Could not find... */
    
    /* Find HID Descriptor */
    for( i = 0; i < in_desc->uc_num_if_tables; i++ ){
        
        if( in_desc->if_tables[i].uc_class == hid_class_num ){
            
            if_tbl = &in_desc->if_tables[i];
            break;
            
        }
        
    }
    if( if_tbl == NULL ) return -EINVAL; /* Could not find... */
    
    /* Copy HID Descriptor */
    if( if_tbl->class_specific_interface_desc_size != 0 ){
        
        memcpy( buf,
                if_tbl->class_specific_interface_desc,
                if_tbl->class_specific_interface_desc_size );
        
    }
    
    /* Calc Packet Size To Be Sent */
    rc = min( ctrl->wLength, (u16)if_tbl->class_specific_interface_desc_size );
    
    return rc;
    
}

int make_report_desc(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf)
{
    
    int ret = 0;
    usb_gadget_desc_table *pstDescTbl;
    
    if( g_core ){
        
        pstDescTbl = g_core->desc_tbl;
        
        if( pstDescTbl ){
            
            if( pstDescTbl->rptdesc.desc_data ){
                
                ret = min( ctrl->wLength, pstDescTbl->rptdesc.us_desc_size );
                memcpy( buf, pstDescTbl->rptdesc.desc_data, ret );
                
            }else{
                
                ret = -EINVAL;
                
            }
            
        }else{
            
            ret = -EINVAL;
            
        }
        
    }else{
        
        ret = -EINVAL;
        
    }
    
    return ret;
    
}

int make_config_desc(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf,
                            u8 desc_type)
{
    int rc;
    void *buf_org;
    usb_gadget_speed_desc *sp_desc;
    usb_gadget_desc_table *desc_tbl;
    usb_gadget_config_desc *cfg_desc;
    usb_gadget_interface_desc *in_desc;
    usb_gadget_if_table *if_tbl;
    usb_gadget_if_table_element *pipe;
    
    struct usb_config_descriptor _cfg_desc;
    struct usb_otg_descriptor _otg_desc;
    struct usb_interface_descriptor _in_desc;
    struct usb_endpoint_descriptor _ep_desc;
    
    u8 desc_index;
    u8 i, d, s;
    
    rc = 0;
    buf_org = buf;
    desc_index = (u8)(ctrl->wValue & 0x00ff);
    
    /* Descriptor Table を取得 */
    desc_tbl = g_core->desc_tbl;
    
    /* Speed Descriptor を取得 */
    sp_desc = NULL;
    if(g_core->gadget->speed == USB_SPEED_HIGH){
        PDEBUG("=================== HS =================== \n");
        if(desc_type == USB_DT_CONFIG){
            sp_desc = &desc_tbl->hs;
        }else if(desc_type == USB_DT_OTHER_SPEED_CONFIG){
            sp_desc = &desc_tbl->lsfs;
        }
    }else{
        PDEBUG("=================== FS =================== \n");
        if(desc_type == USB_DT_CONFIG){
            sp_desc = &desc_tbl->lsfs;
        }else if(desc_type == USB_DT_OTHER_SPEED_CONFIG){
            sp_desc = &desc_tbl->hs;
        }
    }
    if(sp_desc == NULL) return -EINVAL;
    
    /* 指定のindexを持つConfigDescriptorを探す */
    cfg_desc = NULL;
    for(i = 0; i < sp_desc->uc_num_config; i++){
        if(sp_desc->configurations[i].uc_index == desc_index){
            cfg_desc = &sp_desc->configurations[i];
        }
    }
    if(cfg_desc == NULL) return -1; /* 見つからない */
    
    /* Config Descriptor 分を進める */
    rc += (u16)USB_DT_CONFIG_SIZE;
    buf += (u16)USB_DT_CONFIG_SIZE;
    
    /* OTG Descriptorを作る */
    if(cfg_desc->otg){
        _otg_desc.bLength = sizeof(_otg_desc);
        _otg_desc.bDescriptorType = USB_DT_OTG;
        
        _otg_desc.bmAttributes = cfg_desc->otg->uc_attributes;
        
        memcpy(buf, &_otg_desc, _otg_desc.bLength);
        rc += (u16)_otg_desc.bLength;
        buf += (u16)_otg_desc.bLength;
    }
    
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        in_desc = &cfg_desc->interfaces[i];
        
        for(d = 0; d < in_desc->uc_num_if_tables; d++){
            if_tbl = &in_desc->if_tables[d];
            
            /* Interface Descを作る */
            _in_desc.bLength = USB_DT_INTERFACE_SIZE;
            _in_desc.bDescriptorType = USB_DT_INTERFACE;
            
            _in_desc.bInterfaceNumber = in_desc->uc_if_number;
            _in_desc.bAlternateSetting = if_tbl->uc_setting_number;
            _in_desc.bNumEndpoints = if_tbl->uc_num_pep_list;
            _in_desc.bInterfaceClass = if_tbl->uc_class;
            _in_desc.bInterfaceSubClass = if_tbl->uc_sub_class;
            _in_desc.bInterfaceProtocol = if_tbl->uc_interface_protocol;
            _in_desc.iInterface = if_tbl->uc_interface_str;
            
            memcpy(buf, &_in_desc, _in_desc.bLength);
            rc += (u16)_in_desc.bLength;
            buf += (u16)_in_desc.bLength;
            
            /* Class Specific Interface Descriptor のコピー */
            if(if_tbl->class_specific_interface_desc_size != 0){
                memcpy(buf,
                       if_tbl->class_specific_interface_desc,
                       if_tbl->class_specific_interface_desc_size);
                rc += (u16)if_tbl->class_specific_interface_desc_size;
                buf += (u16)if_tbl->class_specific_interface_desc_size;
            }
            
            for(s = 0; s < if_tbl->uc_num_pep_list; s++){
                pipe = &if_tbl->pep_list[s];
                
                /* EP Desc を作る */
                _ep_desc.bLength = USB_DT_ENDPOINT_SIZE;
                _ep_desc.bDescriptorType = USB_DT_ENDPOINT;
                
                _ep_desc.bEndpointAddress = pipe->uc_ep_address;
                _ep_desc.bmAttributes = pipe->uc_attributes;
                _ep_desc.wMaxPacketSize = cpu_to_le16(pipe->us_max_psize);
                _ep_desc.bInterval = pipe->uc_interval;
                
                memcpy(buf, &_ep_desc, _ep_desc.bLength);
                rc += (u16)_ep_desc.bLength;
                buf += (u16)_ep_desc.bLength;
                
                /* Class Specific EP Descriptor のコピー */
                if(pipe->class_specific_ep_desc_size != 0){
                    memcpy(buf,
                           pipe->class_specific_ep_desc,
                           pipe->class_specific_ep_desc_size);
                    rc += (u16)pipe->class_specific_ep_desc_size;
                    buf += (u16)pipe->class_specific_ep_desc_size;
                }
            }
        }
    }
    
    /* ConfigDescriptorを生成 */
    _cfg_desc.bLength = USB_DT_CONFIG_SIZE;
    _cfg_desc.bDescriptorType = desc_type;
    
    _cfg_desc.wTotalLength = rc;
    _cfg_desc.bNumInterfaces = cfg_desc->uc_num_interfaces;
    _cfg_desc.bConfigurationValue = cfg_desc->uc_configuration_value;
    _cfg_desc.iConfiguration = cfg_desc->uc_configuration_str;
    _cfg_desc.bmAttributes = cfg_desc->uc_attributes;
    _cfg_desc.bMaxPower = cfg_desc->uc_max_power;
    
    memcpy(buf_org, &_cfg_desc, _cfg_desc.bLength);
    
    /* 送信するサイズを求める */
    rc = min(ctrl->wLength, (u16)rc);
    
    return rc;
}

int make_device_qualifier_desc(struct g_core_drv *g_core,
                                      const struct usb_ctrlrequest *ctrl,
                                      void *buf)
{
    usb_gadget_desc_table *desc_tbl;
    usb_gadget_speed_desc *sp_desc;
    struct usb_qualifier_descriptor _qualifier_desc;
    int rc;
    
    /* Descriptor Table を取得 */
    desc_tbl = g_core->desc_tbl;
    
    /* Speed Descriptor を取得 */
    if(g_core->gadget->speed == USB_SPEED_HIGH){
        PDEBUG("=================== HS =================== \n");
        sp_desc = &desc_tbl->lsfs;
    }else{
        PDEBUG("=================== FS =================== \n");
        sp_desc = &desc_tbl->hs;
    }
    
    _qualifier_desc.bLength = sizeof(_qualifier_desc);
    _qualifier_desc.bDescriptorType = USB_DT_DEVICE_QUALIFIER;
    
    _qualifier_desc.bcdUSB = cpu_to_le16(sp_desc->us_bcd_usb);
    _qualifier_desc.bDeviceClass = sp_desc->uc_class; 
    _qualifier_desc.bDeviceSubClass = sp_desc->uc_sub_class;
    _qualifier_desc.bDeviceProtocol = sp_desc->uc_protocol;
    _qualifier_desc.bMaxPacketSize0 = g_core->ep0->maxpacket;
    _qualifier_desc.bNumConfigurations = sp_desc->uc_num_config;
    _qualifier_desc.bRESERVED = 0;
    
    rc = min(ctrl->wLength, (u16)sizeof(_qualifier_desc));
    memcpy(buf, &_qualifier_desc, rc);
    
    return rc;
}

int make_string_desc(struct g_core_drv *g_core,
                            const struct usb_ctrlrequest *ctrl,
                            void *buf)
{
    usb_gadget_desc_table *desc_tbl;
    usb_gadget_langid_table *langid_tbl;
    usb_gadget_string_desc *str_desc;
    u8 *pbuf, *org_pbuf;
    u8 desc_index;
    u8 i, len;
    
    /* Descriptor Table を取得 */
    desc_tbl = g_core->desc_tbl;
    
    desc_index = (u8)(ctrl->wValue & 0x00ff);
    org_pbuf = pbuf = (u8*)buf;
    
    PDEBUG("desc_index : %d\n", desc_index);
    if(desc_index == 0){
        /* descriptor 0 has the language id */
        pbuf++;
        *pbuf++ = USB_DT_STRING;
        for(i = 0; i < desc_tbl->uc_num_langids; i++){
            langid_tbl = &desc_tbl->langids[i];
            PDEBUG("langid[%d] : 0x%04x\n", i, (int)langid_tbl->us_langid);
            *pbuf++ = (u8)langid_tbl->us_langid;
            *pbuf++ = (u8)(langid_tbl->us_langid >> 8);
        }
        *org_pbuf = (u8)(pbuf - org_pbuf);  /* Total Length*/
        len = (int)*org_pbuf;
    }
    else if(desc_index == USB_REQ_OS_STRING_DESC_WVALUE_L){
        DEBUG_PRINTK("\x1b[7;35mRecved 0x%x!!\x1b[0m", desc_index);
        /* descriptor 0xEE has OS_STRING_DESC */
        if( g_core->enable_dual_mode != 0 ){
            PDEBUG("now in GET STRING_DESC(0x%x)!!", desc_index );
            len = sizeof(g_os_desc);
            memcpy(pbuf, &g_os_desc, len);
            DEBUG_DUMP(pbuf, len);
        }
        else{
            PDEBUG("now in GET STRING_DESC(0x%x)!!, but dual_mode is diabled!!!!", desc_index );
            return -EINVAL;
        }
    }
    else{
        PDEBUG("langid : 0x%04x\n", ctrl->wIndex);
        
        /* 指定されたlangidのLangidTableを探す */
        langid_tbl = NULL;
        for(i = 0; i < desc_tbl->uc_num_langids; i++){
            if(desc_tbl->langids[i].us_langid == ctrl->wIndex){
                PDEBUG("langid_tbl found!!\n");
                langid_tbl = &desc_tbl->langids[i];
                break;
            }
        }
        if(langid_tbl == NULL) return -EINVAL;
        
        /* 指定されたindex値のStringDescriptorを探す */
        str_desc = NULL;
        for(i = 0; i < langid_tbl->uc_num_strings; i++){
            if(langid_tbl->strings[i].uc_index == desc_index){
                PDEBUG("string_desc found!!\n");
                str_desc = &langid_tbl->strings[i];
                break;
            }
        }
        if(str_desc == NULL) return -EINVAL;
        
        /* StringDescriptorを生成 */
        len = min((__u8)253, str_desc->uc_len);
        if(len != 0){
            memcpy(pbuf + 2, str_desc->string, len);
        }
        len += 2;
        pbuf[0] = len;
        pbuf[1] = USB_DT_STRING;
    }
    
    len = min(ctrl->wLength, (u16)len);
    return len;
}

/*=============================================================================
 * Descriptor Save
 *===========================================================================*/
/* User空間のDescTblを保存する */
int save_desc_tbl(struct g_core_drv *g_core, struct usb_gadgetcore_start_info *_start)
{
    int rs = 0;
    struct usb_gadgetcore_start_info start;
    
    /* usb_gadgetcore_start_info構造体をコピー */
    if(copy_from_user(&start, _start, sizeof(start))){
        rs = -EFAULT;
        goto exit;
    }
    
    /* usb_gadget_desc_table 用の領域を確保 */
    g_core->desc_tbl = kmalloc(sizeof(usb_gadget_desc_table), GFP_KERNEL);
    if(!g_core->desc_tbl){
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)g_core->desc_tbl);
    
    /* usb_gadget_desc_table を内部管理領域へコピー */
    if(copy_from_user(g_core->desc_tbl, start.desc_tbl, sizeof(usb_gadget_desc_table))){
        rs = -EFAULT;
        goto exit;
    }
    
    /* HS Config Descriptor 領域を確保してコピー */
    rs = save_config_desc(&g_core->desc_tbl->hs);
    if(rs != 0){
        goto exit;
    }
    
    /* LSFS Config Descriptor 領域を確保してコピー */
    rs = save_config_desc(&g_core->desc_tbl->lsfs);
    if(rs != 0){
        goto exit;
    }
    
    /* Report Descriptor 領域を確保してコピー */
    rs = save_report_desc( &g_core->desc_tbl->rptdesc );
    if( rs != 0 ){
        goto exit;
    }
    
    /* Langid Table 領域を確保してコピー */
    rs = save_langids(g_core->desc_tbl);
    if(rs != 0){
        goto exit;
    }
    
    rs = overwrite_iserial(g_core->desc_tbl, start.uc_serial_len, start.serial_string);
    if(rs != 0){
        goto exit;
    }

    // Added for DualMode.
    // Get Params from userspace
    g_core->enable_dual_mode = start.ext_info.enable_dual_mode;
    g_core->target_ep_adr    = start.ext_info.target_ep_adr;
    DEBUG_PRINT("### DualMode flag is   ... [%s]", g_core->enable_dual_mode==1 ? "TRUE":"FALSE"  );
    DEBUG_PRINT("### Target EP_ADR is   ... [%d]", g_core->target_ep_adr);

exit:
    if(rs != 0){
        PERR("%s()\n", __func__);
        
        /* 開放 */
        free_desc_tbl(g_core);
    }
    
    return rs;
}

/* Langid Table の保存 */
static int save_langids(usb_gadget_desc_table *desc_tbl)
{
    usb_gadget_langid_table *langids;
    int rs = 0;
    __u8 i;
    
    /* Langid Table の数が0の時は処理成功 */
    if(desc_tbl->uc_num_langids == 0){
        goto exit;
    }
    
    /* Langid Table 領域を確保 */
    langids = kmalloc(sizeof(usb_gadget_langid_table) * desc_tbl->uc_num_langids,
                      GFP_KERNEL);
    if(!langids){
        PDEBUG("kmalloc error\n");
        desc_tbl->langids = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)langids);
    
    /* Langid Table 領域を内部管理領域へコピー */
    if(copy_from_user(langids,
                      desc_tbl->langids,
                      sizeof(usb_gadget_langid_table) * desc_tbl->uc_num_langids)){
        kfree(langids);
        desc_tbl->langids = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    desc_tbl->langids = langids;
    
    for(i = 0; i < desc_tbl->uc_num_langids; i++){
        /* String Descriptor 領域を確保してコピー */
        rs = save_string_desc(&desc_tbl->langids[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
    }
    
exit:
    return rs;
}

/* String Descriptor の保存 */
static int save_string_desc(usb_gadget_langid_table *langids)
{
    usb_gadget_string_desc *str_desc;
    int rs = 0;
    __u8 i;
    
    /* String Descriptor の数が0の時は処理成功 */
    if(langids->uc_num_strings == 0){
        goto exit;
    }
    
    /* String Descriptor 領域を確保 */
    str_desc = kmalloc(sizeof(usb_gadget_string_desc) * langids->uc_num_strings,
                       GFP_KERNEL);
    if(!str_desc){
        PDEBUG("kmalloc error\n");
        langids->strings = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)str_desc);
    
    /* String Descriptor 領域を内部管理領域へコピー */
    if(copy_from_user(str_desc,
                      langids->strings,
                      sizeof(usb_gadget_string_desc) * langids->uc_num_strings)){
        kfree(str_desc);
        langids->strings = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    langids->strings = str_desc;
    
    /* class_specific_ep_desc の領域をコピー */
    for(i = 0; i < langids->uc_num_strings; i++){
        rs = save_strings(&langids->strings[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
    }
    
exit:
    return rs;
}

/* String 領域の保存 */
static int save_strings(usb_gadget_string_desc *str_desc)
{
    void *tmp;
    int rs = 0;
    
    /* Strings のサイズが0の時は処理成功 */
    /*  Descriptorとして正しいのかは別 */
    if(str_desc->uc_len == 0){
        PERR("String Descriptor's String Size is 0\n");
        goto exit;
    }
    
    /* Strings 領域を確保 */
    tmp = kmalloc(str_desc->uc_len, GFP_KERNEL);
    if(!tmp){
        PDEBUG("kmalloc error\n");
        str_desc->string = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)tmp);
    
    /* Strings 領域をコピー */
    if(copy_from_user(tmp,
                      str_desc->string,
                      str_desc->uc_len)){
        kfree(tmp);
        str_desc->string = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    str_desc->string = tmp;
    
exit:
    return rs;
}

static int overwrite_iserial(usb_gadget_desc_table *desc_tbl,
                             __u8 len,
                             void* string)
{
    usb_gadget_langid_table *langid_tbl;
    usb_gadget_string_desc *str_desc;
    void *tmp;
    __u8 i, j, k, flag;
    int rs = 0;
    
    if(string == NULL){
        /* stringがNULLならば正常終了 */
        goto exit;
    }
    
    PDEBUG("start: Overwrite iSerial()\n");
    
    if(desc_tbl->uc_serial_number_str == 0){
        i = 1;
        while(1){
            flag = 0;
            for(j = 0; j < desc_tbl->uc_num_langids; j++){
                langid_tbl = &desc_tbl->langids[j];
                for(k = 0; k < langid_tbl->uc_num_strings; k++){
                    if(i == langid_tbl->strings[k].uc_index){
                        flag = 1;
                    }
                }
            }
            if(flag == 0){
                /* 使っていないindexが見つかった */
                desc_tbl->uc_serial_number_str = i;
                break;
            }
            if(i == 255){
                /* 使っていないindexが見つからなかったら異常終了 */
                rs = -EFAULT;
                goto exit;
            }
            i++;
        }
    }
    
    for(i = 0; i < desc_tbl->uc_num_langids; i++){
        langid_tbl = &desc_tbl->langids[i];
        
        for(j = 0; j < langid_tbl->uc_num_strings; j++){
            if(desc_tbl->uc_serial_number_str ==
               langid_tbl->strings[j].uc_index){
                break;
            }
        }
        
        if(j != langid_tbl->uc_num_strings){
            if(langid_tbl->strings[j].string){
                PDEBUG("%s() : kfree 0x%08x\n",
                       __func__, (unsigned int)langid_tbl->strings[j].string);
                kfree(langid_tbl->strings[j].string);
                langid_tbl->strings[j].string = NULL;
            }
            
            if(len != 0){
                /* String 領域を確保 */
                tmp = kmalloc(len, GFP_KERNEL);
                if(!tmp){
                    PDEBUG("kmalloc error\n");
                    rs = -ENOMEM;
                    goto exit;
                }
                PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)tmp);
                
                /* Strings 領域をコピー */
                if(copy_from_user(tmp, string, len)){
                    kfree(tmp);
                    PDEBUG("copy_from_user error\n");
                    rs = -EFAULT;
                    goto exit;
                }
                
                /* 確保した領域に付け替える */
                langid_tbl->strings[j].string = tmp;
            }
            langid_tbl->strings[j].uc_len = len;
            
        }else{
            if(langid_tbl->uc_num_strings == 255){
                /* 既にlangid内のStringsDescriptorの数が255だったら異常終了 */
                rs = -EFAULT;
                goto exit;
            }
            
            /* String Descriptor 領域を確保 */
            str_desc = kmalloc(sizeof(usb_gadget_string_desc) *
                               (langid_tbl->uc_num_strings + 1),
                                    GFP_KERNEL);
            if(!str_desc){
                PDEBUG("kmalloc error\n");
                rs = -ENOMEM;
                goto exit;
            }
            PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)str_desc);
            
            /* StringsDescriptorテーブルをコピー */
            memcpy(str_desc, langid_tbl->strings,
                   sizeof(usb_gadget_string_desc) * langid_tbl->uc_num_strings);
            PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)langid_tbl->strings);
            kfree(langid_tbl->strings);
            
            /* StringsDescriptor の数をインクリメント */
            langid_tbl->uc_num_strings++;
            
            /* 確保した領域に付け替える */
            langid_tbl->strings = str_desc;
            
            str_desc = &langid_tbl->strings[langid_tbl->uc_num_strings-1];
            
            if(len != 0){
                /* String 領域を確保 */
                tmp = kmalloc(len, GFP_KERNEL);
                if(!tmp){
                    PDEBUG("kmalloc error\n");
                    rs = -ENOMEM;
                    goto exit;
                }
                PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)tmp);
                
                /* Strings 領域をコピー */
                if(copy_from_user(tmp, string, len)){
                    kfree(tmp);
                    PDEBUG("copy_from_user error\n");
                    rs = -EFAULT;
                    goto exit;
                }
                
                /* 確保した領域に付け替える */
                str_desc->string = tmp;
            }
            str_desc->uc_index = desc_tbl->uc_serial_number_str;
            str_desc->uc_len = len;
        }
    }
    
    
exit:
    return rs;
}

/* Config Descriptorの保存 */
static int save_config_desc(usb_gadget_speed_desc *sp_desc)
{
    usb_gadget_config_desc *config_desc;
    int rs = 0;
    __u8 i;
    
    /* Config Descriptor の数が0の時は処理成功 */
    if(sp_desc->uc_num_config == 0){
        goto exit;
    }
    
    /* Config Descriptor 領域を確保 */
    config_desc = kmalloc(sizeof(usb_gadget_config_desc) * sp_desc->uc_num_config,
                          GFP_KERNEL);
    if(!config_desc){
        PDEBUG("kmalloc error\n");
        sp_desc->configurations = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)config_desc);
    
    /* Config Descriptor 領域を内部管理領域へコピー */
    if(copy_from_user(config_desc,
                      sp_desc->configurations,
                      sizeof(usb_gadget_config_desc) * sp_desc->uc_num_config)){
        kfree(config_desc);
        sp_desc->configurations = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs= -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    sp_desc->configurations = config_desc;
    
    for(i = 0; i < sp_desc->uc_num_config; i++){
        /* Interface Descriptor 領域を確保してコピー */
        rs = save_interface_desc(&sp_desc->configurations[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
        
        /* OTG Descriptor 領域を確保してコピー */
        rs = save_otg_desc(&sp_desc->configurations[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
        
        /* IA Descriptor 領域を確保してコピー */
        rs = save_ia_desc(&sp_desc->configurations[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
    }
    
exit:
    return rs;
}

/* Report Descriptorの保存 */
static int save_report_desc( usb_gadget_report_desc *rpt_desc )
{
    
    char *pRptDescData;
    int ret = 0;
    
    /* Success if report descriptor does not exist. */
    if( ( rpt_desc->us_desc_size == 0 ) || ( rpt_desc->desc_data == NULL ) ){
        goto exit;
    }
    
    /* kmalloc for Report Descriptor */
    pRptDescData = kmalloc( rpt_desc->us_desc_size, GFP_KERNEL );
    
    if( !pRptDescData ){
        
        PDEBUG( "kmalloc error. \n" );
        rpt_desc->desc_data = NULL;
        ret = -ENOMEM;
        goto exit;
        
    }
    
    PDEBUG( "%s() : kmalloc 0x%08x\n", __func__, (unsigned int)pRptDescData );
    
    /* Copy Report Descriptor data to internal region. */
    if( copy_from_user( pRptDescData, rpt_desc->desc_data, rpt_desc->us_desc_size ) ){
        
        kfree( pRptDescData );
        rpt_desc->desc_data = NULL;
        PDEBUG( "%s() : copy_from_user error. \n", __func__ );
        ret= -EFAULT;
        goto exit;
        
    }
    
    /* attach with malloc'ed region. */
    rpt_desc->desc_data = pRptDescData;
    
exit:
    return ret;
    
}

/* OTG Descriptorの保存 */
static int save_otg_desc(usb_gadget_config_desc *cfg_desc)
{
    usb_gadget_otg_desc *otg_desc;
    int rs = 0;
    
    /* OTG Descriptor が無い(NULLになっている)ならば処理成功 */
    if(cfg_desc->otg == NULL){
        goto exit;
    }
    
    /* OTG Descriptor 領域を確保 */
    otg_desc = kmalloc(sizeof(usb_gadget_otg_desc), GFP_KERNEL);
    if(!otg_desc){
        PDEBUG("kmalloc error\n");
        /* 失敗したことを示すためにcfg_descを代入しておく */
        /* NULLでは、OTG Descriptorがないのか失敗なのか見分けがつかないため */
        cfg_desc->otg = (usb_gadget_otg_desc*)cfg_desc;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)otg_desc);
    
    /* OTG Descriptor 領域を内部管理領域へコピー */
    if(copy_from_user(otg_desc,
                      cfg_desc->otg,
                      sizeof(usb_gadget_otg_desc))){
        kfree(otg_desc);
        /* 失敗したことを示すためにcfg_descを代入しておく */
        /* NULLでは、OTG Descriptorがないのか失敗なのか見分けがつかないため */
        cfg_desc->otg = (usb_gadget_otg_desc*)cfg_desc;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    cfg_desc->otg = otg_desc;
    
exit:
    return rs;
}

/* Interface Association Descriptorの保存 */
static int save_ia_desc(usb_gadget_config_desc *cfg_desc)
{
    usb_gadget_ia_desc *ia_desc;
    int rs = 0;
    
    /* IA Descriptor の数が0の時は処理成功 */
    if(cfg_desc->uc_num_iads == 0){
        goto exit;
    }
    
    /* IA Descriptor 領域を確保 */
    ia_desc = kmalloc(sizeof(usb_gadget_ia_desc) * cfg_desc->uc_num_iads,
                      GFP_KERNEL);
    if(!ia_desc){
        PDEBUG("kmalloc error\n");
        cfg_desc->iads = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)ia_desc);
    
    /* IA Descriptor 領域を内部管理領域へコピー */
    if(copy_from_user(ia_desc,
                      cfg_desc->iads,
                      sizeof(usb_gadget_ia_desc) * cfg_desc->uc_num_iads)){
        kfree(ia_desc);
        cfg_desc->iads = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    cfg_desc->iads = ia_desc;
    
exit:
    return rs;
}

/* Interface Descriptorの保存 */
static int save_interface_desc(usb_gadget_config_desc *cfg_desc)
{
    usb_gadget_interface_desc *in_desc;
    int rs = 0;
    __u8 i;
    
    /* Interface Descriptor の数が0の時は処理成功 */
    if(cfg_desc->uc_num_interfaces == 0){
        goto exit;
    }
    
    /* Interface Descriptor 領域を確保 */
    in_desc = kmalloc(sizeof(usb_gadget_interface_desc) * cfg_desc->uc_num_interfaces,
                      GFP_KERNEL);
    if(!in_desc){
        PDEBUG("kmalloc error\n");
        cfg_desc->interfaces = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)in_desc);
    
    /* Interface Descriptor 領域を内部管理領域へコピー */
    if(copy_from_user(in_desc,
                      cfg_desc->interfaces,
                      sizeof(usb_gadget_interface_desc) * cfg_desc->uc_num_interfaces)){
        kfree(in_desc);
        cfg_desc->interfaces = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    cfg_desc->interfaces = in_desc;
    
    /* Interface Table 領域を確保してコピー */
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        rs = save_interface_table(&cfg_desc->interfaces[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
    }
    
exit:
    return rs;
}

/* Interface Table の保存 */
static int save_interface_table(usb_gadget_interface_desc *in_desc)
{
    usb_gadget_if_table *if_tbl;
    int rs = 0;
    __u8 i;
    
    /* Interface Table の数が0の時は処理成功 */
    if(in_desc->uc_num_if_tables == 0){
        goto exit;
    }
    
    /* Interface Table 領域を確保 */
    if_tbl = kmalloc(sizeof(usb_gadget_if_table) * in_desc->uc_num_if_tables, 
                     GFP_KERNEL);
    if(!if_tbl){
        PDEBUG("kmalloc error\n");
        in_desc->if_tables = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)if_tbl);
    
    /* Interface Table 領域を内部管理領域へコピー */
    if(copy_from_user(if_tbl,
                      in_desc->if_tables,
                      sizeof(usb_gadget_if_table) * in_desc->uc_num_if_tables)){
        kfree(if_tbl);
        in_desc->if_tables = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    in_desc->if_tables = if_tbl;
    
    for(i = 0; i < in_desc->uc_num_if_tables; i++){
        /* Class Specific Interface Descriptor 領域を確保してコピー */
        rs = save_class_if_desc(&in_desc->if_tables[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
        
        /* Interface Table 領域を確保してコピー */
        rs = save_pep_list(&in_desc->if_tables[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
    }
    
exit:
    return rs;
}

static int save_pep_list(usb_gadget_if_table *if_tbl)
{
    usb_gadget_if_table_element *element;
    int rs = 0;
    __u8 i;
    
    /* Pipe List の数が0の時は処理成功 */
    if(if_tbl->uc_num_pep_list == 0){
        goto exit;
    }
    
    /* Pipe List 領域を確保 */
    element = kmalloc(sizeof(usb_gadget_if_table_element) * if_tbl->uc_num_pep_list, 
                      GFP_KERNEL);
    if(!element){
        PDEBUG("kmalloc error\n");
        if_tbl->pep_list = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)element);
    
    /* Pipe List 領域を内部管理領域へコピー */
    if(copy_from_user(element,
                      if_tbl->pep_list,
                      sizeof(usb_gadget_if_table_element) * if_tbl->uc_num_pep_list)){
        kfree(element);
        if_tbl->pep_list = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    if_tbl->pep_list = element;
    
    /* class_specific_ep_desc の領域をコピー */
    for(i = 0; i < if_tbl->uc_num_pep_list; i++){
        rs = save_class_ep_desc(&if_tbl->pep_list[i]);
        if(rs != 0){
            PDEBUG("%s() error\n", __func__);
            goto exit;
        }
    }
    
exit:
    return rs;
}

/* Class Specific Interface Descriptor の保存 */
static int save_class_if_desc(usb_gadget_if_table *if_tbl)
{
    void *tmp;
    int rs = 0;
    
    /* Class Specific のサイズが0の時は処理成功 */
    if(if_tbl->class_specific_interface_desc_size == 0){
        goto exit;
    }
    
    /* Class Specific 領域を確保 */
    tmp = kmalloc(if_tbl->class_specific_interface_desc_size, GFP_KERNEL);
    if(!tmp){
        PDEBUG("kmalloc error\n");
        if_tbl->class_specific_interface_desc = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)tmp);
    
    /* Class Specific 領域をコピー */
    if(copy_from_user(tmp,
                      if_tbl->class_specific_interface_desc,
                      if_tbl->class_specific_interface_desc_size)){
        kfree(tmp);
        if_tbl->class_specific_interface_desc = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    if_tbl->class_specific_interface_desc = tmp;
    
exit:
    return rs;
}

/* Class Specific EP Descriptor の保存 */
static int save_class_ep_desc(usb_gadget_if_table_element *element)
{
    void *tmp;
    int rs = 0;
    
    if(element->class_specific_ep_desc_size == 0){
        /* Class Specific のサイズが0の時は処理成功 */
        goto exit;
    }
    
    /* Class Specific 領域を確保 */
    tmp = kmalloc(element->class_specific_ep_desc_size, GFP_KERNEL);
    if(!tmp){
        PDEBUG("kmalloc error\n");
        element->class_specific_ep_desc = NULL;
        rs = -ENOMEM;
        goto exit;
    }
    PDEBUG("%s() : kmalloc 0x%08x\n", __func__, (unsigned int)tmp);
    
    /* Class Specific 領域をコピー */
    if(copy_from_user(tmp,
                      element->class_specific_ep_desc,
                      element->class_specific_ep_desc_size)){
        kfree(tmp);
        element->class_specific_ep_desc = NULL;
        PDEBUG("%s() : copy_from_user error\n", __func__);
        rs = -EFAULT;
        goto exit;
    }
    
    /* 確保した領域に付け替える */
    element->class_specific_ep_desc = tmp;
    
exit:
    return rs;
}

/*=============================================================================
 * Free Descriptor
 *===========================================================================*/
void free_desc_tbl(struct g_core_drv *g_core)
{
    int rs = 0;
    
    if(!g_core->desc_tbl){
        goto exit;
    }
    
    /* HS Config Descriptor 領域を開放 */
    rs = free_config_desc(&g_core->desc_tbl->hs);
    if(rs != 0){
        goto exit;
    }
    
    /* LSFS Config Descriptor 領域を開放 */
    rs = free_config_desc(&g_core->desc_tbl->lsfs);
    if(rs != 0){
        goto exit;
    }
    
    /* Report Descriptor 領域を開放 */
    rs = free_report_desc( &g_core->desc_tbl->rptdesc );
    if(rs != 0){
        goto exit;
    }
    
    /* Langid Table 領域を開放 */
    rs = free_langids(g_core->desc_tbl);
    if(rs != 0){
        goto exit;
    }
    
exit:
    if(g_core->desc_tbl){
        /* usb_gadget_desc_table 用の領域を開放 */
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)g_core->desc_tbl);
        kfree(g_core->desc_tbl);
        g_core->desc_tbl = NULL;
    }
}

static int free_langids(usb_gadget_desc_table *desc_tbl)
{
    int rs = 0;
    __u8 i;
    
    for(i = 0; i < desc_tbl->uc_num_langids; i++){
        if(&desc_tbl->langids[i] == NULL){
            PDEBUG("Fail: &desc_tbl->langids[%d] is NULL\n", i);
            rs = -EFAULT;
            goto exit;
        }
        
        rs = free_string_desc(&desc_tbl->langids[i]);
        if(rs != 0){
            rs = -EFAULT;
            goto exit;
        }
    }
    
exit:
    if(desc_tbl->uc_num_langids != 0 && desc_tbl->langids){
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)desc_tbl->langids);
        kfree(desc_tbl->langids);
    }
    
    return rs;
}

static int free_string_desc(usb_gadget_langid_table *langids)
{
    int rs = 0;
    __u8 i;
    
    for(i = 0; i < langids->uc_num_strings; i++){
        if(&langids->strings[i] == NULL){
            PDEBUG("Fail: &langids->strings[%d] is NULL\n", i);
            rs = -EFAULT;
            goto exit;
        }
        
        rs = free_strings(&langids->strings[i]);
        if(rs != 0){
            rs = -EFAULT;
            goto exit;
        }
    }
    
exit:
    if(langids->uc_num_strings != 0 && langids->strings){
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)langids->strings);
        kfree(langids->strings);
    }
    
    return rs;
}

static int free_strings(usb_gadget_string_desc *str_desc)
{
    int rs = 0;
    
    if(str_desc->uc_len == 0){
        goto exit;
    }
    
    if(str_desc->string){
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)str_desc->string);
        kfree(str_desc->string);
    }else{
        rs = -EFAULT;
    }
    
exit:
    return rs;
}

static int free_config_desc(usb_gadget_speed_desc *sp_desc)
{
    int rs = 0;
    __u8 i;
    
    for(i = 0; i < sp_desc->uc_num_config; i++){
        if(&sp_desc->configurations[i] == NULL){
            PDEBUG("Fail: &sp_desc->configurations[%d] is NULL\n", i);
            rs = -EFAULT;
            goto exit;
        }
        
        /* Interface Descriptor 領域を開放 */
        rs = free_interface_desc(&sp_desc->configurations[i]);
        if(rs != 0){
            goto exit;
        }
        
        /* OTG Descriptor 領域を開放 */
        rs = free_otg_desc(&sp_desc->configurations[i]);
        if(rs != 0){
            goto exit;
        }
        
        /* IA Descriptor 領域を開放 */
        rs = free_ia_desc(&sp_desc->configurations[i]);
        if(rs != 0){
            goto exit;
        }
    }
    
exit:
    if(sp_desc->uc_num_config != 0 && sp_desc->configurations){
        /* configurations を開放 */
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)sp_desc->configurations);
        kfree(sp_desc->configurations);
    }
    return rs;
}

static int free_report_desc( usb_gadget_report_desc *rpt_desc )
{
    
    int ret = 0;
    
    if( rpt_desc->desc_data != NULL ){
        
        PDEBUG( "%s() : kfree 0x%08x\n", __func__, (unsigned int)rpt_desc->desc_data );
        kfree( rpt_desc->desc_data );
        rpt_desc->desc_data = NULL;
        
    }
    
    return ret;
    
}

static int free_otg_desc(usb_gadget_config_desc *cfg_desc)
{
    int rs = 0;
    
    if(cfg_desc->otg == NULL){
        goto exit;
    }
    
    if(cfg_desc->otg == (usb_gadget_otg_desc*)cfg_desc){
        rs = -EFAULT;
        goto exit;
    }
    
    PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)cfg_desc->otg);
    kfree(cfg_desc->otg);
    
exit:
    return rs;
}

static int free_ia_desc(usb_gadget_config_desc *cfg_desc)
{
    int rs = 0;
    
    if(cfg_desc->uc_num_iads == 0){
        goto exit;
    }
    
    if(cfg_desc->iads){
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)cfg_desc->iads);
        kfree(cfg_desc->iads);
    }else{
        rs = -EFAULT;
    }
    
exit:
    return rs;
}

static int free_interface_desc(usb_gadget_config_desc *cfg_desc)
{
    int rs = 0;
    __u8 i;
    
    for(i = 0; i < cfg_desc->uc_num_interfaces; i++){
        if(&cfg_desc->interfaces[i] == NULL){
            PDEBUG("Fail: &cfg_desc->interfaces[%d] is NULL\n", i);
            rs = -EFAULT;
            goto exit;
        }
        
        rs = free_interface_table(&cfg_desc->interfaces[i]);
        if(rs != 0){
            goto exit;
        }
    }
    
exit:
    if(cfg_desc->uc_num_interfaces != 0 && cfg_desc->interfaces){
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)cfg_desc->interfaces);
        kfree(cfg_desc->interfaces);
    }
    
    return rs;
}

static int free_interface_table(usb_gadget_interface_desc *in_desc)
{
    int rs = 0;
    __u8 i;
    
    for(i = 0; i < in_desc->uc_num_if_tables; i++){
        if(&in_desc->if_tables[i] == NULL){
            PDEBUG("Fail: &in_desc->if_tables[%d] is NULL\n", i);
            rs = -EFAULT;
            goto exit;
        }
        
        rs = free_class_if_desc(&in_desc->if_tables[i]);
        if(rs != 0){
            goto exit;
        }
        
        rs = free_pep_list(&in_desc->if_tables[i]);
        if(rs != 0){
            goto exit;
        }
    }
    
exit:
    if(in_desc->uc_num_if_tables != 0 && in_desc->if_tables){
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)in_desc->if_tables);
        kfree(in_desc->if_tables);
    }
    
    return rs;
}

static int free_class_if_desc(usb_gadget_if_table *if_tbl)
{
    int rs = 0;
    
    if(if_tbl->class_specific_interface_desc_size == 0){
        goto exit;
    }
    
    if(if_tbl->class_specific_interface_desc){
        PDEBUG("%s() : kfree 0x%08x\n", __func__, 
                    (unsigned int)if_tbl->class_specific_interface_desc);
        kfree(if_tbl->class_specific_interface_desc);
    }else{
        rs = -EFAULT;
    }
    
exit:
    return rs;
}

static int free_pep_list(usb_gadget_if_table *if_tbl)
{
    int rs = 0;
    __u8 i;
    
    for(i = 0; i < if_tbl->uc_num_pep_list; i++){
        if(&if_tbl->pep_list[i] == NULL){
            PDEBUG("Fail: &if_tbl->pep_list[%d] is NULL\n", i);
            rs = -EFAULT;
            goto exit;
        }
        
        rs = free_class_ep_desc(&if_tbl->pep_list[i]);
        if(rs != 0){
            goto exit;
        }
    }
    
exit:
    if(if_tbl->uc_num_pep_list != 0 && if_tbl->pep_list){
        PDEBUG("%s() : kfree 0x%08x\n", __func__, (unsigned int)if_tbl->pep_list);
        kfree(if_tbl->pep_list);
    }
    
    return rs;
}

static int free_class_ep_desc(usb_gadget_if_table_element *element)
{
    int rs = 0;
    
    if(element->class_specific_ep_desc_size == 0){
        goto exit;
    }
    
    if(element->class_specific_ep_desc){
        PDEBUG("%s() : kfree 0x%08x\n", __func__,
                    (unsigned int)element->class_specific_ep_desc);
        kfree(element->class_specific_ep_desc);
    }else{
        rs = -EFAULT;
    }
    
exit:
    return rs;
}

/*=============================================================================
 * Descriptor Dump
 *===========================================================================*/
void dump_device_desc(usb_gadget_desc_table *desc_tbl)
{
    unsigned char c;
    
    /* Descriptor Table 表示開始 */
    printk("Descriptor Table:\n");
    
    /* HS Speed Descriptor 表示 */
    printk("  hs:\n"
           "    us_bcd_usb             0x%04x\n"
           "    uc_class                %5u\n"
           "    uc_sub_class            %5u\n"
           "    uc_protocol             %5u\n"
           "    uc_num_config           %5u\n"
           "    configurations:    0x%08x\n"
           "    uc_otg_attributes:      %5u\n",
           desc_tbl->hs.us_bcd_usb, desc_tbl->hs.uc_class,
           desc_tbl->hs.uc_sub_class, desc_tbl->hs.uc_protocol,
           desc_tbl->hs.uc_num_config, (unsigned int)desc_tbl->hs.configurations,
           desc_tbl->hs.uc_otg_attributes);
    
    /* Config Descriptor 表示 */
    for(c = 0; c < desc_tbl->hs.uc_num_config; c++){
        if((desc_tbl->hs.configurations != NULL) && 
           (&desc_tbl->hs.configurations[c] != NULL)){
            dump_config_desc(&desc_tbl->hs.configurations[c]);
        }
    }
    
    /* LSFS Speed Descriptor 表示 */
    printk("  lsfs:\n"
           "    us_bcd_usb             0x%04x\n"
           "    uc_class                %5u\n"
           "    uc_sub_class            %5u\n"
           "    uc_protocol             %5u\n"
           "    uc_num_config           %5u\n"
           "    configurations:    0x%08x\n"
           "    uc_otg_attributes:      %5u\n",
           desc_tbl->lsfs.us_bcd_usb, desc_tbl->lsfs.uc_class,
           desc_tbl->lsfs.uc_sub_class, desc_tbl->lsfs.uc_protocol,
           desc_tbl->lsfs.uc_num_config, (unsigned int)desc_tbl->lsfs.configurations,
           desc_tbl->lsfs.uc_otg_attributes);

    /* Config Descriptor 表示 */
    for(c = 0; c < desc_tbl->lsfs.uc_num_config; c++){
        if((desc_tbl->lsfs.configurations != NULL) &&
           (&desc_tbl->lsfs.configurations[c] != NULL)){
            dump_config_desc(&desc_tbl->lsfs.configurations[c]);
        }
    }
    
    printk("  us_id_vendor           0x%04x\n"
           "  us_id_product          0x%04x\n"
           "  us_bcd_device          0x%04x\n"
           "  us_default_attributes  0x%04x\n"
           "  uc_manufacturer_str     %5u\n"
           "  uc_product_str          %5u\n"
           "  uc_serial_number_str    %5u\n"
           "  uc_num_langids          %5u\n"
           "  langid_table:      0x%08x\n",
           desc_tbl->us_id_vendor, desc_tbl->us_id_product, 
           desc_tbl->us_bcd_device, desc_tbl->uc_default_attributes,
           desc_tbl->uc_manufacturer_str, desc_tbl->uc_product_str, 
           desc_tbl->uc_serial_number_str, desc_tbl->uc_num_langids, 
           (unsigned int)desc_tbl->langids);

    /* Langid Table 表示 */
    for(c = 0; c < desc_tbl->uc_num_langids; c++){
        if((desc_tbl->langids != NULL) &&
           (&desc_tbl->langids[c] != NULL)){
            dump_langid_table(&desc_tbl->langids[c]);
        }
    }
}

static void dump_config_desc(usb_gadget_config_desc *desc)
{
    unsigned char c;
    
    /* Config Descriptor 表示 */
    printk("      uc_index                %5u\n"
           "      uc_configuration_value  %5u\n"
           "      uc_configuration_str    %5u\n"
           "      uc_max_power            %5u\n"
           "      otg                0x%08x\n",
           desc->uc_index, desc->uc_configuration_value,
           desc->uc_configuration_str, desc->uc_max_power,
           (unsigned int)desc->otg);
    
    /* OTG Descriptor 表示 */
    if(desc->otg){
        dump_otg_desc(desc->otg);
    }
    
    printk("      uc_num_iads             %5u\n"
           "      iads               0x%08x\n",
           desc->uc_num_iads, (unsigned int)desc->iads);

    /* Interface Association Descriptor 表示 */
    if(desc->iads){
        dump_ia_desc(desc->iads);
    }
    
    printk("      uc_num_interfaces       %5u\n"
           "      interfaces         0x%08x\n",
           desc->uc_num_interfaces, (unsigned int)desc->interfaces);
    
    /* Interface Descriptor 表示 */
    for(c = 0; c < desc->uc_num_interfaces; c++){
        if((desc->interfaces != NULL) && (&desc->interfaces[c]) != NULL){
            dump_interface_desc(&desc->interfaces[c]);
        }
    }
}

static void dump_otg_desc(usb_gadget_otg_desc *desc)
{
    /* OTG Descriptor 表示 */
    printk("        uc_attributes          0x%04x\n",
           desc->uc_attributes);
}

static void dump_ia_desc(usb_gadget_ia_desc *desc)
{
    /* Interface Association Descriptor 表示 */
    printk("        uc_firstinterface       %5u\n"
           "        uc_interface_count      %5u\n"
           "        uc_function_class       %5u\n"
           "        uc_function_sub_class   %5u\n"
           "        uc_function_protocol    %5u\n"
           "        uc_function             %5u\n",
           desc->uc_firstinterface,
           desc->uc_interface_count,
           desc->uc_function_class,
           desc->uc_function_sub_class,
           desc->uc_function_protocol,
           desc->uc_function);
}

static void dump_interface_desc(usb_gadget_interface_desc *desc)
{
    unsigned char c;
    
    /* Interface Descriptor 表示 */
    printk("        uc_if_number            %5u\n"
           "        uc_num_if_tables        %5u\n"
           "        if_tables          0x%08x\n",
           desc->uc_if_number,
           desc->uc_num_if_tables,
           (unsigned int)desc->if_tables);
    
    /* Interface Table 表示 */
    for(c = 0; c < desc->uc_num_if_tables; c++){
        if((desc->if_tables != NULL) && (&desc->if_tables[c] != NULL)){
            dump_if_tables(&desc->if_tables[c]);
        }
    }
}

static void dump_if_tables(usb_gadget_if_table *if_tbl)
{
    unsigned char c;
    __u32 i;
    
    /* Interface Table 表示 */
    printk("          uc_setting_number       %5u\n"
           "          uc_class                %5u\n"
           "          uc_sub_class            %5u\n"
           "          uc_interface_protocol   %5u\n"
           "          uc_interface_str        %5u\n"
           "          class_specific_interface_desc_size\n"
           "                                  %5u\n"
           "          class_specific_interface_desc\n"
           "                             0x%08x\n",
           if_tbl->uc_setting_number,
           if_tbl->uc_class,
           if_tbl->uc_sub_class,
           if_tbl->uc_interface_protocol,
           if_tbl->uc_interface_str,
           if_tbl->class_specific_interface_desc_size,
           (unsigned int)if_tbl->class_specific_interface_desc);

    /* class_specific_interface_desc を表示する */
    for(i = 0; i < if_tbl->class_specific_interface_desc_size; i++){
        if(i % 16 == 0){ printk("\n            "); }
        printk("0x%02x ", ((unsigned char*)if_tbl->class_specific_interface_desc)[i]);
    }
    if(if_tbl->class_specific_interface_desc_size != 0){ printk("\n"); }
    
    
    printk("          uc_num_pep_list         %5u\n"
           "          pep_list           0x%08x\n",
           if_tbl->uc_num_pep_list,
           (unsigned int)if_tbl->pep_list);

    /* Pipe List 表示 */
    for(c = 0; c < if_tbl->uc_num_pep_list; c++){
        if((if_tbl->pep_list != NULL) && (&(if_tbl->pep_list[c]) != NULL)){
            dump_pep_list(&if_tbl->pep_list[c]);
        }
    }
}

static void dump_pep_list(usb_gadget_if_table_element *pep_list)
{
    __u32 i;
    
    /* Pipe List 表示 */
    printk("            uc_ep_address           %5u\n"
           "            uc_attributes           %5u\n"
           "            us_max_psize            %5u\n"
           "            uc_interval             %5u\n"
           "            class_specific_ep_desc_size\n"
           "                                  %5u\n"
           "            class_specific_ep_desc\n"
           "                             0x%08x\n",
           pep_list->uc_ep_address,
           pep_list->uc_attributes,
           pep_list->us_max_psize,
           pep_list->uc_interval,
           pep_list->class_specific_ep_desc_size,
           (unsigned int)pep_list->class_specific_ep_desc);
    
    /* class_specific_ep_desc を表示する */
    for(i = 0; i < pep_list->class_specific_ep_desc_size; i++){
        if(i % 16 == 0){ printk("\n            "); }
        printk("0x%02x ", ((unsigned char*)pep_list->class_specific_ep_desc)[i]);
    }
    if(pep_list->class_specific_ep_desc_size != 0){ printk("\n"); }
}

static void dump_langid_table(usb_gadget_langid_table *tbl)
{
    unsigned char c;
    
    /* Langid Table 表示 */
    printk("      us_langid               %5u\n"
           "      uc_num_strings          %5u\n"
           "      strings            0x%08x\n",
           tbl->us_langid,
           tbl->uc_num_strings,
           (unsigned int)tbl->strings);
    
    /* String Descriptor 表示 */
    for(c = 0; c < tbl->uc_num_strings; c++){
        if((tbl->strings != NULL) && (&tbl->strings[c] != NULL)){
            dump_string_desc(&tbl->strings[c]);
        }
    }
}

static void dump_string_desc(usb_gadget_string_desc *str_desc)
{
    unsigned char c;
    
    printk("        uc_index                %5u\n"
           "        uc_len                  %5u\n"
           "        strings            0x%08x\n",
           str_desc->uc_index,
           str_desc->uc_len,
           (unsigned int)str_desc->string);
    
    /* Strings 表示 */
    printk("\"");
    for(c = 0; c < str_desc->uc_len; c++){
        printk("%c", ((unsigned char*)str_desc->string)[c]);
    }
    printk("\"\n\n");
}
