/*
 * arch/arm/mach-cxd90014/cxd90014.c
 *
 * CXD90014 devices
 *
 * Copyright 2012 Sony Corporation
 * Copyright (C) 2011-2012 FUJITSU SEMICONDUCTOR LIMITED
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
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <mach/uart.h>
#include <asm/io.h>
#include <asm/pmu.h>
#include <mach/gic_export.h>
#include <mach/cxd90014_board.h>
#include <mach/cxd90014_cfg.h>
#include <mach/regs-misc.h>
#include <mach/bam.h>
#include <mach/pcie_export.h>
#include <mach/dap_export.h>
#include <mach/nandc_export.h>

#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)
#include <linux/smsc911x.h>
#include <mach/regs-sramc.h>
#include <mach/regs-gpio.h>
#include <mach/regs-octrl.h>
#endif

/*
 * NAME
 * 	gpiopin_to_irq
 * SYNOPSIS
 *	#include <linux/interrupt.h>
 *	int gpiopin_to_irq(unsigned int port, unsigned int bit);
 * INPUT
 *	port: GPIO port number (0=GPIO_S, 1=GPIO_SB)
 *       bit: GPIO bit number (0..31)
 * RETURN VALUE
 *      IRQ number
 *	  If input is out of range, -1 shall be returned.
 */
int gpiopin_to_irq(unsigned int port, unsigned int bit)
{
	int irq = -1;

	switch (port) {
	case 0: /* GPIO_S */
		if (bit < NR_GPIO_S_IRQ) {	/* GPIO_S_0 .. 15 */
			irq = IRQ_GPIO_BASE + bit;
		} else if (16 == bit) {		/* GPIO_S_16 */
			irq = IRQ_GPIO_S_16;
		}
		break;
	case 1: /* GPIO_SB */
		if (bit < NR_GPIO_SB_IRQ) {	/* GPIO_SB_0 .. 6 */
			irq = IRQ_GPIO_SB_BASE + bit;
		}
		break;
	default:
		break;
	}
	return irq;
}
EXPORT_SYMBOL(gpiopin_to_irq);

/* List of edge trigger IRQs, except GPIO */
static u8 edge_irqs[] __initdata = {
#ifdef CONFIG_MPCORE_GIC_INIT
	IRQ_NANDC,
	IRQ_MS0_INS_RISE, IRQ_MS0_INS_FALL,
	IRQ_USB_VBUS, IRQ_USB_ID,
#endif /* CONFIG_MPCORE_GIC_INIT */
};

/* List of GPIO IRQ definitions */
#define GPIOIRQ_EDGE	0x01 /* edge */
#define GPIOIRQ_INV	0x02 /* active L */
#define G_RISE	GPIOIRQ_EDGE
#define G_FALL	(GPIOIRQ_EDGE|GPIOIRQ_INV)
static u8 gpio_irqs[] __initdata = {
#ifdef CONFIG_MPCORE_GIC_INIT
	/* 0 */	G_RISE,	G_RISE,	G_RISE,	G_FALL,
		G_RISE,	G_RISE,	G_RISE,	G_RISE,
	/* 8 */ G_RISE,	G_RISE,	G_RISE,	G_RISE,
		G_RISE,	G_RISE, G_RISE,	G_RISE,
	/*16 */	G_RISE,	G_RISE,	G_RISE, G_RISE,
		G_RISE,	G_RISE,	G_RISE,	G_RISE,
#endif /* CONFIG_MPCORE_GIC_INIT */
};

void __init cxd90014_irq_type_init(void)
{
	int i;
	u8 *p;

	/* setup edge trigger IRQs, except GPIO */
	for (i = 0, p = edge_irqs; i < sizeof edge_irqs; i++, p++) {
		gic_config_edge((unsigned int)*p);
	}
	/* setup GPIO IRQs */
	for (i = 0, p = gpio_irqs; i < sizeof gpio_irqs; i++, p++) {
		u8 typ;
		typ = *p;
		if (typ & GPIOIRQ_EDGE) { /* edge */
			gic_config_edge((unsigned int)(IRQ_GPIO_BASE + i));
			/* set EXINT */
			if (typ & GPIOIRQ_INV) { /* fall edge */
				writel_relaxed(1 << i, VA_MISC+MISC_GPIOINTLE+MISCREG_SET);
			} else { /* rise edge */
				writel_relaxed(1 << i, VA_MISC+MISC_GPIOINTHE+MISCREG_SET);
			}

		} else { /* level */
			/* set EXINT */
			writel_relaxed(1 << i, VA_MISC+MISC_GPIOINTS+MISCREG_SET);
			if (typ & GPIOIRQ_INV) { /* active Low */
				writel_relaxed(1 << 1, VA_MISC+MISC_GPIOINTINV+MISCREG_SET);
			}
		}
	}
}

static struct resource pmu_resources[] = {
	[0] = {
		.start	= IRQ_PMU(0),
		.end	= IRQ_PMU(0),
		.flags	= IORESOURCE_IRQ,
	},
	[1] = {
		.start	= IRQ_PMU(1),
		.end	= IRQ_PMU(1),
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= IRQ_PMU(2),
		.end	= IRQ_PMU(2),
		.flags	= IORESOURCE_IRQ,
	},
	[3] = {
		.start	= IRQ_PMU(3),
		.end	= IRQ_PMU(3),
		.flags	= IORESOURCE_IRQ,
	},
};

static irqreturn_t cxd90014_pmu_handler(int irq, void *dev, irq_handler_t handler)
{
	cxd90014_cti_ack(irq - IRQ_PMU_BASE);
	return handler(irq, dev);
}

static struct arm_pmu_platdata cxd90014_pmu_platdata = {
	.handle_irq	= cxd90014_pmu_handler,
};

static struct platform_device pmu_device = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.num_resources	= ARRAY_SIZE(pmu_resources),
	.resource	= pmu_resources,
	.dev.platform_data	= &cxd90014_pmu_platdata,
};


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
		.init_name = "uart0",
		.platform_data	= &uart0_data,
	},
	.res = {
		 .start = CXD90014_UART(0),
		 .end   = CXD90014_UART(0)+SZ_4K-1,
		 .flags = IORESOURCE_MEM,
	 },
	.irq = { IRQ_UART(0), },
};

static struct amba_device uart1_device = {
	.dev = {
		.init_name = "uart1",
		.platform_data	= &uart1_data,
	},
	.res = {
		 .start = CXD90014_UART(1),
		 .end   = CXD90014_UART(1)+SZ_4K-1,
		 .flags = IORESOURCE_MEM,
	 },
	.irq = { IRQ_UART(1), },
};

static struct amba_device uart2_device = {
	.dev = {
		.init_name = "uart2",
		.platform_data	= &uart2_data,
	},
	.res = {
		 .start = CXD90014_UART(2),
		 .end   = CXD90014_UART(2)+SZ_4K-1,
		 .flags = IORESOURCE_MEM,
	 },
	.irq = { IRQ_UART(2), },
};

static struct amba_device *uart_devs[] __initdata = {
	&uart0_device,
	&uart1_device,
	&uart2_device,
};

#if !defined(CONFIG_QEMU)
/* USB Host controller */
static u64 f_usb20ho_ehci_dma_mask = 0xffffffff;
static u64 f_usb20ho_ohci_dma_mask = 0xffffffff;

static struct resource f_usb20ho_ehci_resources[] = {
	[0] = {
		.start	= CXD90014_EHCI_BASE,
		.end	= CXD90014_EHCI_BASE + CXD90014_EHCI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB_EHCI,
		.end	= IRQ_USB_EHCI,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device f_usb20ho_ehci_device = {
	.name           = "f_usb20ho_ehci",
	.id             = 0,
	.dev = {
		.dma_mask           = &f_usb20ho_ehci_dma_mask,
		.coherent_dma_mask  = ~(u32)0,
	},
	.num_resources  = ARRAY_SIZE(f_usb20ho_ehci_resources),
	.resource       = f_usb20ho_ehci_resources,
};

static struct resource f_usb20ho_ohci_resources[] = {
	[0] = {
		.start	= CXD90014_OHCI_BASE,
		.end	= CXD90014_OHCI_BASE + CXD90014_OHCI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB_OHCI,
		.end	= IRQ_USB_OHCI,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device f_usb20ho_ohci_device = {
	.name           = "f_usb20ho_ohci",
	.id             = 0,
	.dev = {
		.dma_mask           = &f_usb20ho_ohci_dma_mask,
		.coherent_dma_mask  = ~(u32)0,
	},
	.num_resources  = ARRAY_SIZE(f_usb20ho_ohci_resources),
	.resource       = f_usb20ho_ohci_resources,
};

/* USB gadget */
static struct resource f_usb20hdc_udc_usb0_resources[] = {
	[0] = {
		.start	= CXD90014_USBG_BASE,
		.end	= CXD90014_USBG_BASE + CXD90014_USBG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB_CINTR,
		.end	= IRQ_USB_CINTR,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= IRQ_USB_DIRQ0,
		.end	= IRQ_USB_DIRQ0,
		.flags	= IORESOURCE_IRQ,
	},
	[3] = {
		.start	= IRQ_USB_DIRQ1,
		.end	= IRQ_USB_DIRQ1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device f_usb20hdc_udc_usb0_device = {
	.name           = "f_usb20hdc_udc",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(f_usb20hdc_udc_usb0_resources),
	.resource       = f_usb20hdc_udc_usb0_resources,
};

static struct platform_device *fusb_devices[] __initdata = {
	&f_usb20ho_ehci_device,
	&f_usb20ho_ohci_device,
	&f_usb20hdc_udc_usb0_device,
};
#endif /* !CONFIG_QEMU */

static int bam_enable = 1;
module_param_named(bam, bam_enable, bool, S_IRUSR);

static struct resource bam_resources[] = {
	[0] = {
		.start  = CXD90014_BAM_BASE,
		.end	= CXD90014_BAM_BASE+CXD90014_BAM_SIZE-1,
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

#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)

#define SMC911X_INT		CONFIG_CXD90014_SMC911X_INT
#define SMC9118_MEM_START	CXD90014_SRAMC_CS0_BASE
static struct resource smc9118_resources[] = {
	[0] = {
		.start	= SMC9118_MEM_START,
		.end	= SMC9118_MEM_START + SZ_4K-1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_GPIO(SMC911X_INT),
		.end	= IRQ_GPIO(SMC911X_INT),
		.flags	= IORESOURCE_IRQ | IRQF_TRIGGER_RISING,
	}
};

static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= 0,  /* active low */
	.irq_type	= 1,
#ifdef CONFIG_CXD90014_SRAMC_CSMASK_ERRATA
	.flags		= SMSC911X_USE_16BIT,
#else
	.flags		= SMSC911X_USE_32BIT,
#endif /* CONFIG_CXD90014_SRAMC_CSMASK_ERRATA */
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

#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)
#define VA_SRAMC(ch,reg)	(sramc + SRAMC_REG(ch,reg))
/* SRAMC_CS# of ETHER */
#define ETHER_CS 0
/* config CS0 as DW=16bit */
#define SRAMC_ETHER_MODE	(SRAMC_MODE_DW16)

#ifdef CONFIG_CXD90014_FPGA
/* CLK=25MHz (period=40ns) */
/* RIDLEC=1, FRADC=16, RADC=1, RACC=3 */
#define SRAMC_ETHER_RDTIM	0x00100102
/* WIDLEC=1, WWEC=2, WADC=1, WACC=4 */
#define SRAMC_ETHER_WRTIM	0x00010003
/* RADHDC=0, RADVC=1, RADSUC=0 */
#define SRAMC_ETHER_RADV	0x00000100
/* WADHDC=0, WADVC=1, WADSUC=0 */
#define SRAMC_ETHER_WADV	0x00000100

#else /* ASIC */
/* CLK=126MHz (period=7.9ns) */
#ifdef CONFIG_CXD90014_SRAMC_CSMASK_ERRATA
/* RIDLEC+1=3, FRADC=16(fix), RADC=5, RACC+1=19 */
#define SRAMC_ETHER_RDTIM	0x02100512
#else
/* RIDLEC+1=1, FRADC=16(fix), RADC=5, RACC+1=19 */
#define SRAMC_ETHER_RDTIM	0x00100512
#endif /* CONFIG_CXD90014_SRAMC_CSMASK_ERRATA */
/* WIDLEC+1=1, WWEC+1=9, WADC+1=5, WACC+1=20 */
#define SRAMC_ETHER_WRTIM	0x00080413
/* WCSMWC=17, RCSMWC=16 */
#define SRAMC_ETHER_CSMWC	0x01110110
/* RADHDC=2, RADVC=2, RADSUC=0 */
#define SRAMC_ETHER_RADV	0x00020200
/* WADHDC=2, WADVC=2, WADSUC=0 */
#define SRAMC_ETHER_WADV	0x00020200
#endif /* CONFIG_CXD90014_FPGA */

/* IRQ of ETHER */
#define ETHER_GPIO 0
#endif /* CONFIG_SMSC911X */

void cxd90014_board_init(void)
{
#if defined(CONFIG_HW_PERF_EVENTS)
	cxd90014_cti_setup();
#endif /* CONFIG_HW_PERF_EVENTS */
#if !defined(CONFIG_CXD90014_FPGA) && !defined(CONFIG_QEMU)
	cxd90014_pcie_setup();
#ifdef CONFIG_CXD90014_NANDC
	cxd90014_nandc_setconfig();
#endif
	bam_powersave(true);
#endif /* !CONFIG_CXD90014_FPGA && !CONFIG_QEMU */

#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)
	{
	void __iomem *sramc;
	sramc = ioremap_nocache(CXD90014_SRAMC_BASE, CXD90014_SRAMC_SIZE);
	if (!sramc) {
		printk("ERROR:cxd90014_board_init: ioremap failed\n");
		return;
	}
	/*--- setup SRAMC for ETHER ---*/
	/* MODE   */
	writel_relaxed(SRAMC_ETHER_MODE, VA_SRAMC(ETHER_CS,MODE));
	/* async read timing */
	writel_relaxed(SRAMC_ETHER_RDTIM, VA_SRAMC(ETHER_CS,RTIM));
	/* async write timing */
	writel_relaxed(SRAMC_ETHER_WRTIM, VA_SRAMC(ETHER_CS,WTIM));
#ifndef CONFIG_CXD90014_SRAMC_CSMASK_ERRATA
	/* async csmask timinig */
	writel_relaxed(SRAMC_ETHER_CSMWC, VA_SRAMC(ETHER_CS,CSMWC));
#endif /* !CONFIG_CXD90014_SRAMC_CSMASK_ERRATA */
	/* async read adv timing */
	writel_relaxed(SRAMC_ETHER_RADV, VA_SRAMC(ETHER_CS,RADV));
	/* async write adv timing */
	writel_relaxed(SRAMC_ETHER_WADV, VA_SRAMC(ETHER_CS,WADV));
	/* dummy read */
	readl_relaxed(VA_SRAMC(ETHER_CS,WADV));
	iounmap(sramc);

#ifdef CONFIG_CXD90014_FPGA
	/*--- select ASYNC2 pin ---*/
	writel_relaxed(OCTRL_ASYNCSEL, VA_ASYNCSEL+OCTRL_SET);
#endif /* CONFIG_CXD90014_FPGA */

	/*--- setup INT pin ---*/
	/* DIC (setup by request_irq) */
	/* GPIO */
	/*   fall edge */
	writel_relaxed(1 << cxd90014_ether_irq, VA_MISC+MISC_GPIOINTS+MISCREG_CLR);
	writel_relaxed(1 << cxd90014_ether_irq, VA_MISC+MISC_GPIOINTHE+MISCREG_CLR);
	writel_relaxed(1 << cxd90014_ether_irq, VA_MISC+MISC_GPIOINTLE+MISCREG_SET);
#if !defined(CONFIG_CXD90014_FPGA)
	/* pull up */
	writel_relaxed(1 << cxd90014_ether_irq, VA_OCTRL(ETHER_GPIO)+OCTRL_PUD2+OCTRL_SET);
	/* pull on */
	writel_relaxed(1 << cxd90014_ether_irq, VA_OCTRL(ETHER_GPIO)+OCTRL_PUD1+OCTRL_SET);
#endif /* !CONFIG_CXD90014_FPGA */
	/*   input enable */
	writel_relaxed(1 << cxd90014_ether_irq, VA_GPIO(ETHER_GPIO,INEN)+GPIO_SET);
	}
#endif /* CONFIG_SMSC911X */
}

static void __init cxd90014_fixup(struct machine_desc *desc,
				 struct tag *tags, char **cmdline,
				 struct meminfo *mi)
{
}

static void __init cxd90014_init(void)
{
	int i, ret;
	struct amba_device **dev;

	cxd90014_board_init();
	ret = cxd90014_core_init();
	for (i = 0, dev = uart_devs; i < ARRAY_SIZE(uart_devs); i++, dev++) {
		if (cxd90014_uart_bitmap & (1 << i)) {
			ret = amba_device_register(*dev, &iomem_resource);
			if (ret) {
				printk(KERN_ERR "cxd90014_init: amba_device_register(uart_devs[%d]) failed.\n", i);
			}
		}
	}
	platform_device_register(&pmu_device);

#if !defined(CONFIG_QEMU)
	/* USB devices */
	ret = platform_add_devices(fusb_devices, ARRAY_SIZE(fusb_devices));
	if (ret) {
		printk(KERN_ERR "f_usb20hdc_udc_usb0_device add failed.\n");
	}
#endif /* !CONFIG_QEMU */

	/* BAM */
	if (bam_enable) {
		ret = platform_device_register(&bam_device);
		if (ret) {
			printk(KERN_ERR "cxd90014_init: platform_device_register bam failed.\n");
		}
	}
#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)
	{ /* Override IRQ pin */
		smc9118_resources[1].start = IRQ_GPIO(cxd90014_ether_irq);
		smc9118_resources[1].end   = IRQ_GPIO(cxd90014_ether_irq);
	}
	ret = platform_device_register(&smc9118_device);
	if (ret) {
		printk(KERN_ERR "cxd90014_init: platform_device_register smc9118 failed.\n");
	}
#endif /* CONFIG_SMSC911X */
}

MACHINE_START(CXD90014, "ARM-CXD90014")
	/* Maintainer: Sony Corporation */
	.map_io		= cxd90014_map_io,
	.init_irq	= cxd90014_init_irq,
	.timer		= &cxd4115_timer,
	.init_machine	= cxd90014_init,
	.fixup		= cxd90014_fixup,
#ifdef CONFIG_QEMU
	.boot_params= PHYS_OFFSET + 0X0100,
#endif
MACHINE_END
