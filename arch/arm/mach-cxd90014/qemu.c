/*
 * arch/arm/mach-cxd90014/qemu.c
 *
 * Copyright 2012,2013 Sony Corporation
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 */
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/clk.h>
#include <linux/amba/clcd.h>
#include <linux/dma-mapping.h>

#include <asm/clkdev.h>

#include "clock.h"

#define SMC91X_IRQ	240
#define SMC91X_MEM	0xf3000000

#define CLCD_MEM 0xf4000000
#define CLCD_IRQ 241

/*
 * Ether
 */
static struct resource smc91x_resources[] = {
	[0] = {
		.start	= SMC91X_MEM,
		.end	= SMC91X_MEM + SZ_4K-1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = SMC91X_IRQ,
		.end	= SMC91X_IRQ,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smc91x_resources),
	.resource	= smc91x_resources,
};

/*
 * CLCD
 */
static struct clcd_panel vga = {
	.mode		= {
		.name		= "VGA",
		.refresh	= 60,
		.xres		= 640,
		.yres		= 480,
		.pixclock	= 39721,
		.left_margin	= 64,
		.right_margin	= 16,
		.upper_margin	= 13,
		.lower_margin	= 3,
		.hsync_len	= 80,
		.vsync_len	= 4,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD | TIM2_IPC,
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

static struct clcd_panel *qemu_clcd_panel(void)
{
	return &vga;
}

static int qemu_clcd_setup(struct clcd_fb *fb)
{
	unsigned long framesize;
	dma_addr_t dma;

	/* VGA, max 32bpp */
	framesize = 640 * 480 * 4;

	fb->panel		= qemu_clcd_panel();

	fb->fb.screen_base = dma_alloc_writecombine(&fb->dev->dev, framesize,
						    &dma, GFP_KERNEL);
	if (!fb->fb.screen_base) {
		printk(KERN_ERR "CLCD: unable to map framebuffer\n");
		return -ENOMEM;
	}

	fb->fb.fix.smem_start	= dma;
	fb->fb.fix.smem_len	= framesize;

	return 0;
}

static int qemu_clcd_mmap(struct clcd_fb *fb, struct vm_area_struct *vma)
{
	return dma_mmap_writecombine(&fb->dev->dev, vma,
				     fb->fb.screen_base,
				     fb->fb.fix.smem_start,
				     fb->fb.fix.smem_len);
}

static void qemu_clcd_remove(struct clcd_fb *fb)
{
	dma_free_writecombine(&fb->dev->dev, fb->fb.fix.smem_len,
			      fb->fb.screen_base, fb->fb.fix.smem_start);
}

static struct clcd_board clcd_data = {
	.name		= "CXD90014",
	.check		= clcdfb_check,
	.decode		= clcdfb_decode,
	.setup		= qemu_clcd_setup,
	.mmap		= qemu_clcd_mmap,
	.remove		= qemu_clcd_remove,
};

static struct amba_device clcd_device = {
	.dev = {
		.init_name = "apb:clcd",
		.platform_data	= &clcd_data,
		.coherent_dma_mask = ~0,
	},
	.res = {
		 .start = CLCD_MEM,
		 .end   = CLCD_MEM+SZ_4K-1,
		 .flags = IORESOURCE_MEM,
	 },
	.irq = { CLCD_IRQ, },
	.dma_mask = ~0,
};

static struct amba_device *amba_devs[] __initdata = {
	&clcd_device,
};

/*
 * ================ CLOCK ========================
 */
static int qemu_clk_enable(struct clk *clk, int enable)
{
	return 0;
}

static unsigned long qemu_clk_getrate(struct clk *clk)
{
	return 1;
}

static struct clk qemu_clk = {
	.name		= "QEMU",
	.chan		= 2,
	.get_rate       = qemu_clk_getrate,
	.enable		= qemu_clk_enable,
};

static struct clk_lookup lookups[] = {
#ifdef CONFIG_QEMU
	{	/* clcd */
		.dev_id		= "apb:clcd",
		.clk		= &qemu_clk,
	},
#endif
};

static int __init qemu_init(void)
{
	int i, ret;
	struct amba_device **dev;

	for (i = 0; i < ARRAY_SIZE(lookups); i++)
		clkdev_add(&lookups[i]);

	for (i = 0, dev = amba_devs; i < ARRAY_SIZE(amba_devs); i++, dev++) {
		ret = amba_device_register(*dev, &iomem_resource);
		if (ret) {
			printk(KERN_ERR "cxd4115_init: amba_device_register failed.\n");
		}
	}

	ret = platform_device_register(&smc91x_device);
	if (ret) {
		printk(KERN_ERR "cxd4115_init: platform_device_register failed.\n");
	}

	return 0;
}
arch_initcall(qemu_init);
