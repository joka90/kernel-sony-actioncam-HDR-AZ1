/*
 * arch/arm/mach-cxd4132/cxd4132.c
 *
 * CXD4132 devices
 *
 * Copyright 2009,2010 Sony Corporation
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
 *
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <mach/uart.h>
#include <asm/io.h>
#include <mach/regs-fsp.h>
#include <mach/regs-gpio.h>
#include <mach/cxd4132_board.h>
#include <mach/cxd41xx_cfg.h>
#include <mach/powersave.h>

#ifdef CONFIG_SMSC911X
#include <linux/smsc911x.h>
#include <mach/regs-octrl.h>
#endif

static struct uart_platform_data uart0_data = {
	.chan		= 0,
	.fifosize	= 32,
	.clock		= "UARTCLK0",
};

static struct uart_platform_data uart1_data = {
	.chan		= 1,
	.fifosize	= 32,
	.clock		= "UARTCLK1",
};

static struct uart_platform_data uart2_data = {
	.chan		= 2,
	.fifosize	= 32,
	.clock		= "UARTCLK2",
};

static struct amba_device uart0_device = {
	.dev = {
		.init_name = "apb:050",
		.platform_data	= &uart0_data,
	},
	.res = {
		 .start = CXD4115_UART(0),
		 .end   = CXD4115_UART(0)+SZ_4K-1,
		 .flags = IORESOURCE_MEM,
	 },
	.irq = { IRQ_UART(0), },
};

static struct amba_device uart1_device = {
	.dev = {
		.init_name = "apb:051",
		.platform_data	= &uart1_data,
	},
	.res = {
		 .start = CXD4115_UART(1),
		 .end   = CXD4115_UART(1)+SZ_4K-1,
		 .flags = IORESOURCE_MEM,
	 },
	.irq = { IRQ_UART(1), },
};

static struct amba_device uart2_device = {
	.dev = {
		.init_name = "apb:052",
		.platform_data	= &uart2_data,
	},
	.res = {
		 .start = CXD4115_UART(2),
		 .end   = CXD4115_UART(2)+SZ_4K-1,
		 .flags = IORESOURCE_MEM,
	 },
	.irq = { IRQ_UART(2), },
};

static struct amba_device *uart_devs[] __initdata = {
	&uart0_device,
	&uart1_device,
	&uart2_device,
};

static int fcs_enable = 1;
module_param_named(fcs, fcs_enable, bool, S_IRUSR);

static struct resource fcs_resources[] = {
	[0] = {
		.start  = IRQ_DFS,
		.end	= IRQ_DFS,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device fcs_device = {
	.name		= "fcs",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(fcs_resources),
	.resource	= fcs_resources,
};

static int bam_enable = 1;
module_param_named(bam, bam_enable, bool, S_IRUSR);

static struct resource bam_resources[] = {
	[0] = {
		.start  = CXD4132_BAM_BASE,
		.end	= CXD4132_BAM_BASE+CXD4132_BAM_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_AVB,
		.end	= IRQ_AVB,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device bam_device = {
	.name		= "bam",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(bam_resources),
	.resource	= bam_resources,
};

#if defined(CONFIG_SMSC911X)

#define SMC911X_INT	CONFIG_CXD41XX_SMC911X_INT
#define SMC9118_MEM_START	FSP_CS1_BASE
static struct resource smc9118_resources[] = {
	[0] = {
		.start	= SMC9118_MEM_START,
		.end	= SMC9118_MEM_START + SZ_4K-1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_GPIO_FALL(SMC911X_INT),
		.end	= IRQ_GPIO_FALL(SMC911X_INT),
		.flags	= IORESOURCE_IRQ,
	}
};

static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= 0,  /* active low */
	.irq_type	= 1,
	.flags		= SMSC911X_USE_32BIT,
};

static struct platform_device smc9118_device = {
	.name		= "smsc911x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smc9118_resources),
	.resource	= smc9118_resources,
	.dev = {
		.platform_data = &smsc911x_config,
	},
};
#endif /* CONFIG_SMSC911X */

#if defined(CONFIG_SMSC911X)
/* FSP_PHY_CS# of ETHER */
#define FSP_ETHER_CS 1
/* config CS1 as MBSWP,ASYNC,PageMode=off,DW=16bit */
#define FSP_ETHER_CONFIG (FSP_PHY_CFG1_MBSWP|FSP_PHY_CFG1_DW16)
/* CLK=192MHz (period=5.2ns) */
/* TWADVN=2, TWEA=3, TWEN=9, TWDH=2, TWWAIT=1 */
#define FSP_ETHER_PARAM1 0x43090201
/* TRADVN=2, TOEA=3, TRDLAT=11, TRAH=0, TRWAIT=4 */
#define FSP_ETHER_PARAM2 0x430b0004
/* IRQ of ETHER */
#define GPIO_ETHER_IRQ 0
#endif /* CONFIG_SMSC911X */

void cxd4132_board_init(void)
{
	/* setup FSP CS1 I/O window */
	writel(FSP_CS1_BASE,                VA_FSP+FSP_PHY_START(1));
	writel(FSP_CS1_BASE+FSP_CS1_SIZE-1, VA_FSP+FSP_PHY_END(1));
	/* setup FSP CS2 I/O window */
	writel(FSP_CS2_BASE,                VA_FSP+FSP_PHY_START(2));
	writel(FSP_CS2_BASE+FSP_CS2_SIZE-1, VA_FSP+FSP_PHY_END(2));

	/* Powersave */
	bam_powersave(true);

#if defined(CONFIG_SMSC911X)
	{
	/*--- setup FSP for ETHER ---*/
	/* config   */
	writel(FSP_ETHER_CONFIG, VA_FSP+FSP_PHY(FSP_ETHER_CS,CFG1));
	/* automatic sync mode: OFF */
	writel(0, VA_FSP+FSP_PHY(FSP_ETHER_CS,WBSETBIT));
	writel(0, VA_FSP+FSP_PHY(FSP_ETHER_CS,RBSETBIT));
	/* async write timing */
	writel(FSP_ETHER_PARAM1, VA_FSP+FSP_PHY(FSP_ETHER_CS,PRM1));
	/* async read timing */
	writel(FSP_ETHER_PARAM2, VA_FSP+FSP_PHY(FSP_ETHER_CS,PRM2));

	/*--- setup INT pin ---*/
	/* DIC (setup by gic.c or config) */
	/* GPIO */
	/*   pullup */
	writel(1 << cxd41xx_ether_irq, VA_OCTRL(GPIO_ETHER_IRQ)+OCTRL_PUD2+OCTRL_SET);
	/*   pull on */
	writel(1 << cxd41xx_ether_irq, VA_OCTRL(GPIO_ETHER_IRQ)+OCTRL_PUD1+OCTRL_CLR);
	/*   input enable */
	writel(1 << cxd41xx_ether_irq, VA_GPIO(GPIO_ETHER_IRQ)+GPIO_INEN_SET);
	}
#endif /* CONFIG_SMSC911X */
}

static void __init cxd4132_fixup(struct machine_desc *desc,
				 struct tag *tags, char **cmdline,
				 struct meminfo *mi)
{
}

static void __init cxd4132_init(void)
{
	int i, ret;
	struct amba_device **dev;

	/* SMODE */
	if (CXD41XX_PORT_UNDEF == cxd41xx_smode_port) {
		cxd41xx_smode = -1;
	} else {
		unsigned int port, bit;
		port = cxd41xx_smode_port & 0xff;
		bit  = (cxd41xx_smode_port >> 8) & 0xff;
		cxd41xx_smode = !!(readl(VA_GPIO(port)+GPIO_DATA_READ) & (1 << bit));
		printk(KERN_INFO "SMODE: %d\n", cxd41xx_smode);
        }
	cxd4132_board_init();
	ret = cxd4132_core_init();
	for (i = 0, dev = uart_devs; i < ARRAY_SIZE(uart_devs); i++, dev++) {
		if (cxd41xx_uart_bitmap & (1 << i)) {
			ret = amba_device_register(*dev, &iomem_resource);
			if (ret) {
				printk(KERN_ERR "cxd4132_init: amba_device_register(uart_devs[%d]) failed.\n", i);
			}
		}
	}
	if (bam_enable) {
		ret = platform_device_register(&bam_device);
		if (ret) {
			printk(KERN_ERR "cxd4132_init: platform_device_register bam failed.\n");
		}
	}
	if (fcs_enable) {
		ret = platform_device_register(&fcs_device);
		if (ret) {
			printk(KERN_ERR "cxd4132_init: platform_device_register fcs failed.\n");
		}
	}
#if defined(CONFIG_SMSC911X)
	{ /* Override IRQ pin */
		smc9118_resources[1].start = IRQ_GPIO_FALL(cxd41xx_ether_irq);
		smc9118_resources[1].end   = IRQ_GPIO_FALL(cxd41xx_ether_irq);
	}
	ret = platform_device_register(&smc9118_device);
	if (ret) {
		printk(KERN_ERR "cxd4132_init: platform_device_register smc9118 failed.\n");
	}
#endif /* CONFIG_SMSC911X */
}

MACHINE_START(CXD4132, "ARM-CXD4132")
	/* Maintainer: Sony Corporation */
	.map_io		= cxd4132_map_io,
	.init_irq	= cxd4132_init_irq,
	.timer		= &cxd4115_timer,
	.init_machine	= cxd4132_init,
	.fixup		= cxd4132_fixup,
#ifdef CONFIG_QEMU
	.boot_params	= PHYS_OFFSET + 0x0100,
#endif
MACHINE_END
