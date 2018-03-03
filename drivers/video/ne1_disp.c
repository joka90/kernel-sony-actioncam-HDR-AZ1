/* 2009-07-22: File added and changed by Sony Corporation */
/*
 *  File Name		: drivers/video/ne1_disp.c
 *  Function		: display controller
 *  Release Version : Ver 1.00
 *  Release Date	: 2008/06/25
 *
 *  Copyright (C) NEC Electronics Corporation 2008
 *
 *
 *  This program is free software;you can redistribute it and/or modify it under the terms of
 *  the GNU General Public License as published by Free Softwere Foundation; either version 2
 *  of License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warrnty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with this program;
 *  If not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/mach-types.h>
#include <mach/ne1_sysctrl.h>
#include <asm/gpio.h>

#include "ne1_disp.h"

#define NE1_HW_LAYER_0	0x8c000000ULL
#define NE1_HW_LAYER_1	0x8c800000ULL
#define NE1_HW_LAYER_2	0x8d000000ULL
#define NE1_HW_LAYER_3	0x8d800000ULL
#define NE1_HW_LAYER_4	0x8e000000ULL
#define NE1_HW_LAYER_5	0x8e800000ULL
#define NE1_HW_LAYER_6	0x8f000000ULL
#define NE1_FB_BASE	NE1_HW_LAYER_0
#define NE1_FB_MAX_LEN	0x00800000

#define NE1_FB_DEF_16

typedef struct {
	u8 addr;
	u8 data;
} stlcd_init_data;

typedef struct {
	u32 x_res, y_res;
	u32 bpp;
	u32 dot_clock;
	u32 hsw, hfp, hbp;
	u32 vsw, vfp, vbp;
	u32 use_lcd_init;
	u32 lcd_init_data_num;
	stlcd_init_data *lcd_init_data;
	int reset_port;
	u32 reset_level;
} ne1_lcd_info;

#ifdef CONFIG_FB_NE1_LCD
static stlcd_init_data lcd_data[] = {
	{  3, 0x01}, {  0, 0x00}, {  1, 0x01}, {  4, 0x00}, {  5, 0x14},	// 5-9
	{  6, 0x24}, { 16, 0xD7}, { 17, 0x00}, { 18, 0x00}, { 19, 0x55},	// 10-14
	{ 20, 0x01}, { 21, 0x70}, { 22, 0x1E}, { 23, 0x25}, { 24, 0x25},	// 15-19
	{ 25, 0x02}, { 26, 0x02}, { 27, 0xA0}, { 32, 0x2F}, { 33, 0x0F},	// 20-24
	{ 34, 0x0F}, { 35, 0x0F}, { 36, 0x0F}, { 37, 0x0F}, { 38, 0x0F},	// 25-29
	{ 39, 0x00}, { 40, 0x02}, { 41, 0x02}, { 42, 0x02}, { 43, 0x0F},	// 30-34
	{ 44, 0x0F}, { 45, 0x0F}, { 46, 0x0F}, { 47, 0x0F}, { 48, 0x0F},	// 35-39
	{ 49, 0x0F}, { 50, 0x00}, { 51, 0x02}, { 52, 0x02}, { 53, 0x02},	// 40-44
	{ 80, 0x0C}, { 83, 0x42}, { 84, 0x42}, { 85, 0x41}, { 86, 0x14},	// 45-49
	{ 89, 0x88}, { 90, 0x01}, { 91, 0x00}, { 92, 0x02}, { 93, 0x0C},	// 50-54
	{ 94, 0x1C}, { 95, 0x27}, { 98, 0x49}, { 99, 0x27}, {102, 0x76},	// 55-59
	{103, 0x27}, {112, 0x01}, {113, 0x0E}, {114, 0x02}, {115, 0x0C},	// 60-64
	{118, 0x0C}, {121, 0x30}, {130, 0x00}, {131, 0x00}, {132, 0xFC},	// 65-69
	{134, 0x00}, {136, 0x00}, {138, 0x00}, {139, 0x00}, {140, 0x00},	// 70-74
	{141, 0xFC}, {143, 0x00}, {145, 0x00}, {147, 0x00}, {148, 0x00},	// 75-79
	{149, 0x00}, {150, 0xFC}, {152, 0x00}, {154, 0x00}, {156, 0x00},	// 80-84
	{157, 0x00}, {2, 0x00}, 	// 85, 88
};

static ne1_lcd_info lcd_info = {
	.x_res		= 800,
	.y_res		= 480,
	.bpp		= 24,
	.dot_clock	= 23800000,
	.hsw		= 1,
	.hfp		= 6,
	.hbp		= 4,
	.vsw		= 1,
	.vfp		= 3,
	.vbp		= 4,
	.use_lcd_init = 1,
	.lcd_init_data_num = 82,
	.lcd_init_data = lcd_data,
	.reset_port = -1,
	.reset_level = 1,
};
#else

#ifdef CONFIG_FB_NE1_RES_WXGA
static ne1_lcd_info lcd_info = {
	.x_res		= 1280,
	.y_res		= 768,
	.bpp		= 24,
	.dot_clock	= 80140000,
	.hsw		= 36,
	.hfp		= 64,
	.hbp		= 200,
	.vsw		= 3,
	.vfp		= 1,
	.vbp		= 23,
	.use_lcd_init = 0,
	.reset_port = -1,
};
#elif CONFIG_FB_NE1_RES_XGA
static ne1_lcd_info lcd_info = {
	.x_res		= 1024,
	.y_res		= 768,
	.bpp		= 24,
	.dot_clock	= 64110000,
	.hsw		= 104,
	.hfp		= 56,
	.hbp		= 160,
	.vsw		= 3,
	.vfp		= 1,
	.vbp		= 23,
	.use_lcd_init = 0,
	.reset_port = -1,
};
#elif CONFIG_FB_NE1_RES_SVGA
static ne1_lcd_info lcd_info = {
	.x_res		= 800,
	.y_res		= 600,
	.bpp		= 24,
	.dot_clock	= 38220000,
	.hsw		= 80,
	.hfp		= 32,
	.hbp		= 112,
	.vsw		= 3,
	.vfp		= 1,
	.vbp		= 18,
	.use_lcd_init = 0,
	.reset_port = -1,
};
#elif CONFIG_FB_NE1_RES_VGA
static ne1_lcd_info lcd_info = {
	.x_res		= 640,
	.y_res		= 480,
	.bpp		= 24,
	.dot_clock	= 25200000,
	.hsw		= 96,
	.hfp		= 16,
	.hbp		= 48,
	.vsw		= 2,
	.vfp		= 10,
	.vbp		= 33,
	.use_lcd_init = 0,
	.reset_port = -1,
};
#else
#error "Set lcd_info for resolution"
#endif

#endif

struct ne1_fb_par {
	struct device	*dev;

	u32		pseudo_palette[256];

	void __iomem	*regs;		/* remapped registers */

	struct clk	*clk;
	int		blanked;
	u32		dot_clock;
};

struct {
        u32 offset;
        u64 data;
} HWdefaults[] = {
	{ IF_CNT_OTA,           0x0000000000000000ULL},
	{ DS_BRIGHT,            0x0000000000000000ULL},
	{ DS_DITHER,            0x0000000000000000ULL},
	{ DF_BGC,               0x0000000000000000ULL},
	{ DF_CDC_PER,           0x0000000000000000ULL},
	{ INT_PERM,             0x0000000000000000ULL}, /* Disable Interrupts */
	{ MF_ESWP_BEX,          0x0000000000000000ULL}, /* 1024 x 7 layer */
	{ MF_DFSPX_03,          0x0000000000000000ULL},
	{ MF_DFSPY_03,          0x0000000000000000ULL},
	{ MF_DFSPYF,            0x0000000000000000ULL},
	{ DF_SPX_03,            0x0000000000000000ULL},
	{ DF_SPY_03,            0x0000000000000000ULL},
	{ DF_LTRANS_03,         0x0000000000000000ULL},
	{ DF_CKEY_CNT,          0x0000000000000000ULL},
	{ IF_CNT_OTA,           0x0000000000000004ULL},
};

static u64 ne1_fb_read64(void __iomem *pos)
{
	u64 val;
	__asm__ __volatile__ ("ldrd %0, [%1]\n" :"=r" (val): "r" (pos));

	return val;
}

static void ne1_fb_write64(u64 val, void __iomem *pos)
{
	__asm__ __volatile__ ("strd %0, [%1]\n": : "r" (val), "r" (pos));
}


static int lcd_init_data_send(u8 addr, u8 data)
{
	// T.B.D
	return 0;
}

static void ne1_fb_setup_hw(struct fb_info *info)
{
	int i, ret;
	struct ne1_fb_par *par = info->par;
	u64 val;

	// Initialized LCD
	if (lcd_info.use_lcd_init) {
		// Unreset LCD
		if (lcd_info.reset_port != -1) {
			gpio_direction_output(lcd_info.reset_port, (lcd_info.reset_level) ? 0 : 1);
		}
		// Send LCD init data
		for (i = 0; i < lcd_info.lcd_init_data_num; i++) {
			ret = lcd_init_data_send(lcd_info.lcd_init_data[i].addr, lcd_info.lcd_init_data[i].data);
			if (ret < 0) {
				printk("%s: Send LCD init data ERROR!(%d)", __FUNCTION__, ret);
			}
		}
	}

	// Setting clock
	clk_set_rate(par->clk, lcd_info.dot_clock);
	par->dot_clock = clk_get_rate(par->clk);
	info->var.pixclock = 10000000 / (par->dot_clock / 100000);

	clk_enable(par->clk);
	// Unreset
	writel(readl(SYSCTRL_BASE + SYSCTRL_SRST_PERI) & ~SYSCTRL_SRST_PERI_DISP, SYSCTRL_BASE + SYSCTRL_SRST_PERI);

	for (i=0; i < sizeof(HWdefaults)/sizeof(HWdefaults[0]); i++) {
		ne1_fb_write64(HWdefaults[i].data, par->regs + HWdefaults[i].offset);
	}

	// Set Resolution
	val = NE1_SET_DS_TV((u64)(lcd_info.y_res + lcd_info.vsw + lcd_info.vfp + lcd_info.vbp)) |
			NE1_SET_DS_TH((u64)(lcd_info.x_res + lcd_info.hsw + lcd_info.hfp + lcd_info.hbp)) |
			NE1_SET_DS_VR(lcd_info.y_res) | NE1_SET_DS_HR(lcd_info.x_res);
	ne1_fb_write64(val, par->regs + DS_RSLT_SYNC);

	// Set layer size
	if (lcd_info.x_res > 1024) {
		val = ne1_fb_read64(par->regs + MF_ESWP_BEX);
		val |= 1 << NE1_SHIFT_MF_LBS;
		ne1_fb_write64(val, par->regs + MF_ESWP_BEX);
	}

	// Set Pulse width
	val = NE1_SET_DS_TVB((u64)lcd_info.vbp) | NE1_SET_DS_THB((u64)lcd_info.hbp) |
			NE1_SET_DS_TVP((u64)lcd_info.vsw) | NE1_SET_DS_THP((u64)lcd_info.hsw);
	ne1_fb_write64(val, par->regs + DS_PLSW_BP);

	// Set Format
#ifdef NE1_FB_DEF_16
	val = NE1_SET_MF_PF0(NE1_PIXFMT_ARGB0565);
#else
	val = NE1_SET_MF_PF0(NE1_PIXFMT_ARGB8888);
#endif
	ne1_fb_write64(val, par->regs + MF_PIXFMT_03);

	// Set buffer start address
	ne1_fb_write64((NE1_HW_LAYER_1 << 32) | NE1_HW_LAYER_0, par->regs + MF_SADR_01);
	ne1_fb_write64((NE1_HW_LAYER_3 << 32) | NE1_HW_LAYER_2, par->regs + MF_SADR_23);
	ne1_fb_write64((NE1_HW_LAYER_5 << 32) | NE1_HW_LAYER_4, par->regs + MF_SADR_45);
	ne1_fb_write64(NE1_HW_LAYER_6, par->regs + MF_SADR_6);

	// Set widh/height size
	val = NE1_SET_MF_WS_H0(lcd_info.x_res - 1);
	ne1_fb_write64(val, par->regs + MF_SIZEH_03);
	val = NE1_SET_MF_DS_H0(lcd_info.x_res - 1);
	ne1_fb_write64(val, par->regs + MF_DFSIZEH_03);
	val = NE1_SET_MF_WS_V0(lcd_info.y_res - 1);
	ne1_fb_write64(val, par->regs + MF_SIZEV_03);
	val = NE1_SET_MF_DS_V0(lcd_info.y_res - 1);
	ne1_fb_write64(val, par->regs + MF_DFSIZEV_03);

	// Enable L0 layer
	val = NE1_SET_DF_LC0(1) | NE1_SET_DF_VS0(0) | NE1_SET_DF_DC0(7);
	ne1_fb_write64(val, par->regs + DF_CNT_03);

	// Enable LCD output
	switch (lcd_info.hbp) {
	case 16:
		val = NE1_SET_DS_DS(3) | NE1_SET_DS_OC(1);
		break;
	case 18:
		val = NE1_SET_DS_DS(2) | NE1_SET_DS_OC(1);
		break;
	case 24:
		val = NE1_SET_DS_DS(1) | NE1_SET_DS_OC(1);
		break;
	default:
		printk("%s: Unsupported display. output bit=%d\n", __FUNCTION__, lcd_info.hbp);
		break;
	}
	ne1_fb_write64(val, par->regs + DS_SET);
}

static int ne1_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	switch (var->bits_per_pixel) {
	case 8:
		var->red.offset    = 0;
		var->red.length    = 8;
		var->green.offset  = 0;
		var->green.length  = 8;
		var->blue.offset   = 0;
		var->blue.length   = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 16:	/* RGB 565 */
		var->red.offset    = 11;
		var->red.length    = 5;
		var->green.offset  = 5;
		var->green.length  = 6;
		var->blue.offset   = 0;
		var->blue.length   = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 32:		/* ARGB 8888 */
		var->red.offset    = 16;
		var->red.length    = 8;
		var->green.offset  = 8;
		var->green.length  = 8;
		var->blue.offset   = 0;
		var->blue.length   = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ne1_fb_set_par(struct fb_info *info)
{
	struct ne1_fb_par *par = info->par;
	struct fb_var_screeninfo *var = &info->var;
	u64 fmt = ne1_fb_read64(par->regs + MF_PIXFMT_03) & ~NE1_MASK_MF_PF0;
	u64 ds_set = ne1_fb_read64(par->regs + DS_SET);

	switch(var->bits_per_pixel) {
	case 8:
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
		break;
	case 16:
		info->fix.visual = FB_VISUAL_DIRECTCOLOR;
		ne1_fb_write64(ds_set & ~NE1_MASK_DS_OC, par->regs + DS_SET);
		if (ds_set & NE1_MASK_DS_OC) {
			while ((ne1_fb_read64(par->regs + INT_STAT_VPC) & NE1_MASK_IC_VSS) == 0);
		}
		ne1_fb_write64(fmt | NE1_SET_MF_PF0(NE1_PIXFMT_ARGB0565), par->regs + MF_PIXFMT_03);
		ne1_fb_write64(ds_set, par->regs + DS_SET);
		break;
	case 32:
		info->fix.visual = FB_VISUAL_TRUECOLOR;
		ne1_fb_write64(ds_set & ~NE1_MASK_DS_OC, par->regs + DS_SET);
		if (ds_set & NE1_MASK_DS_OC) {
			while ((ne1_fb_read64(par->regs + INT_STAT_VPC) & NE1_MASK_IC_VSS) == 0);
		}
		ne1_fb_write64(fmt | NE1_SET_MF_PF0(NE1_PIXFMT_ARGB8888), par->regs + MF_PIXFMT_03);
		ne1_fb_write64(ds_set, par->regs + DS_SET);
		break;
	}

	info->fix.line_length = var->xres * var->bits_per_pixel / 8;

	return 0;
}


static int ne1_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
                        unsigned blue, unsigned transp, struct fb_info *info)
{
	u32 *pal;

	switch(info->var.bits_per_pixel) {
	case 8:
		break;
	case 16:
		if (regno >= 256) {
			return 1;
		}
		pal = info->pseudo_palette;     // System has 16 default color
		pal[regno] = (red & 0xf800) | ((green & 0xfc00) >> 5) | ((blue & 0xf800) >> 11);
		break;
	case 32:
		if (regno >= 256) {
			return 1;
		}
		pal = info->pseudo_palette;     // System has 16 default color
		pal[regno] = ((red & 0xff00) << 8) | (green & 0xff00) | ((blue & 0xff00) >> 8);
		break;
	default:
		return 1;
	}

	return 0;
}

static int ne1_fb_blank(int blank_mode, struct fb_info *info)
{
	struct ne1_fb_par *par = info->par;
	u64 ds_set;

	if ((blank_mode) && (!par->blanked)) {
		ds_set = ne1_fb_read64(par->regs + DS_SET);
		ne1_fb_write64(ds_set & ~NE1_MASK_DS_OC, par->regs + DS_SET);
		par->blanked = 1;
	} else if ((!blank_mode) && (par->blanked)) {
		ds_set = ne1_fb_read64(par->regs + DS_SET);
		ne1_fb_write64(ds_set | NE1_SET_DS_OC(1), par->regs + DS_SET);
		par->blanked = 0;
	}

	return 0;
}


static struct fb_ops ne1_fb_fbops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= ne1_fb_check_var,
	.fb_setcolreg	= ne1_fb_setcolreg,
	.fb_blank		= ne1_fb_blank,
	.fb_set_par     = ne1_fb_set_par,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static void __devinit ne1_fb_init_info(struct fb_info *info)
{
	info->fix.type           = FB_TYPE_PACKED_PIXELS;
	info->fix.type_aux       = 0;
	info->fix.xpanstep       = 0;
	info->fix.ypanstep       = 0;
	info->fix.ywrapstep      = 0;
	info->fix.accel          = FB_ACCEL_NONE;

	info->var.nonstd         = 0;
	info->var.activate       = FB_ACTIVATE_NOW;
#ifdef NE1_FB_DEF_16
	info->var.bits_per_pixel = 16;
#else
	info->var.bits_per_pixel = 32;
#endif
	info->var.height         = -1;
	info->var.width          = -1;
	info->var.accel_flags    = 0;
	info->var.vmode          = FB_VMODE_NONINTERLACED;

	info->fbops = &ne1_fb_fbops;
	info->node               = -1;
	info->flags = FBINFO_DEFAULT;

	info->fix.mmio_start = NE1_DISPLAY_REGBASE;
	info->fix.mmio_len = NE1_DISPLAY_REGSIZE;
	info->fix.smem_start = NE1_FB_BASE;
	info->fix.smem_len =  NE1_FB_MAX_LEN;

}

static void ne1_fb_set_pars(struct fb_info *info, struct ne1_fb_par *par)
{
	struct fb_var_screeninfo *var = &info->var;
	struct fb_fix_screeninfo *fix = &info->fix;

	fix->type = FB_TYPE_PACKED_PIXELS;

	switch(var->bits_per_pixel) {
	case 8:
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
		break;
	case 16:
		info->fix.visual = FB_VISUAL_DIRECTCOLOR;
		break;
	case 32:
		info->fix.visual = FB_VISUAL_TRUECOLOR;
		break;
	}

	var->xres = lcd_info.x_res;
	var->yres = lcd_info.y_res;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres;
	var->xoffset = 0;
	var->yoffset = 0;

	var->left_margin	= lcd_info.hfp;
	var->right_margin	= lcd_info.hbp;
	var->upper_margin	= lcd_info.vfp;
	var->lower_margin	= lcd_info.vbp;
	var->hsync_len		= lcd_info.hsw;
	var->vsync_len		= lcd_info.vsw;

	fix->line_length = var->xres * info->var.bits_per_pixel / 8;
	fix->xpanstep = 0;
	fix->ypanstep = 0;
	fix->accel = FB_ACCEL_NONE;

	ne1_fb_setup_hw(info);
}

static int __init ne1_fb_probe(struct platform_device *pdev)
{
	struct device     *dev = &pdev->dev;
	struct ne1_fb_par *par;
	struct fb_info    *info;
	int ret = 0;

	info = framebuffer_alloc(sizeof(struct ne1_fb_par), dev);
	if (!info) {
		return -ENOMEM;
	}

	par = info->par;
	memset(par, 0, sizeof(struct ne1_fb_par));
	par->regs = ioremap(NE1_DISPLAY_REGBASE, NE1_DISPLAY_REGSIZE);
	if (!par->regs) {
		ret = -ENOMEM;
		goto init_fail;
	}
	info->pseudo_palette = par->pseudo_palette;

	info->screen_base = ioremap(NE1_FB_BASE, NE1_FB_MAX_LEN);
	if (!info->screen_base) {
		ret = -ENOMEM;
		goto init_fail;
	}

	printk(KERN_INFO "ne1_fb: regs mapped at %p, fb mapped at %p\n",
		par->regs, info->screen_base);

	strcpy(info->fix.id, "NE1 display");

	info->pseudo_palette     = par->pseudo_palette;

	ne1_fb_init_info(info);

	// Clear buffer
	memset(info->screen_base, 0, info->var.xres * info->var.yres * 4);

	par->clk = clk_get(NULL, "DISP");
	if (IS_ERR(par->clk)) {
		goto init_fail;
	}

	ne1_fb_set_pars(info, par);

	fb_alloc_cmap(&info->cmap, 256, 0);
	fb_set_cmap(&info->cmap, info);
	ne1_fb_check_var(&info->var, info);

	if (register_framebuffer(info) < 0) {
		ret = -EINVAL;
		goto init_fail;
	}

	platform_set_drvdata(pdev, info);

	printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node, info->fix.id);

	return 0;

init_fail:
	if (info) {
		if (par->clk)
			clk_put(par->clk);
		if (info->screen_base)
			iounmap(info->screen_base);
		if (par->regs)
			iounmap(par->regs);
		framebuffer_release(info);
	}
	return ret;
}

static int ne1_fb_remove(struct platform_device *pdev)
{
	struct fb_info    *info = platform_get_drvdata(pdev);
	struct ne1_fb_par *par  = info->par;

	if (par->clk) {
		clk_disable(par->clk);
		clk_put(par->clk);
	}

	if (par) {
		if (par->regs)
			iounmap(par->regs);
	}
	if (info) {
		fb_dealloc_cmap(&info->cmap);
		if (info->screen_base)
			iounmap(info->screen_base);
		framebuffer_release(info);
	}

	return 0;
}

#ifdef CONFIG_PM
static int ne1_fb_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int ne1_fb_resume(struct platform_device *pdev)
{
	return 0;
}
#endif


static struct platform_driver ne1_fb_driver = {
	.probe		= ne1_fb_probe,
	.remove		= ne1_fb_remove,
#ifdef CONFIG_PM
	.suspend	= ne1_fb_suspend,
	.resume		= ne1_fb_resume,
#endif
	.driver		= {
		.name	= "ne1_fb",
		.owner	= THIS_MODULE,
	},
};

static struct platform_device *ne1_fb_device;
static int __devinit ne1_fb_init(void)
{
	int ret;

	ret = platform_driver_register(&ne1_fb_driver);
	if (!ret) {
		ne1_fb_device = platform_device_alloc("ne1_fb", 0);
		if (ne1_fb_device)
			ret = platform_device_add(ne1_fb_device);
		else
			ret= -ENOMEM;

		if (ret) {
			platform_device_put(ne1_fb_device);
			platform_driver_unregister(&ne1_fb_driver);
		}
	}
	return ret;
}

static void __exit ne1_fb_cleanup(void)
{
	platform_device_unregister(ne1_fb_device);
	platform_driver_unregister(&ne1_fb_driver);
}

module_init(ne1_fb_init);
module_exit(ne1_fb_cleanup);
MODULE_LICENSE("GPL");


