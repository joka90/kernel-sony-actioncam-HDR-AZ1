/* 2008-09-16: File added and changed by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/core.c
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on arch/arm/mach-realview/core.c
 *
 * Copyright (C) 1999 - 2003 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/serial_8250.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/cnt32_to_63.h>
#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)
#include <linux/smsc911x.h>
#else
#include <linux/smc911x.h>
#endif

#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/leds.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <linux/amba/mmci.h>

#include <asm/hardware/gic.h>
#include <mach/ne1_timer.h>
#include <mach/ne1tb.h>


#include "core.h"
#include "clock.h"

#if 0
/* used by entry-macro.S */
void __iomem *gic_cpu_base_addr;
#endif

#define NE1xB_REFCOUNTER                (IO_ADDRESS(NE1_BASE_TIMER_1)  + TMR_TMRCNT)

/*
 * This is the NE1-xB sched_clock implementation.
 *
 * NE1-xB uses the onboard timer 1 which provides an 32 bit value.
 * The counter increments at 66.666MHZ frequency so its
 * resolution of 15 ns.
 */
#define CYC2NS_SCALE_FACTOR 		10

/* make sure COUNTER_CLOCK_TICK_RATE % TICK_RATE_SCALE_FACTOR == 0 */
#define TICK_RATE_SCALE_FACTOR 		1000

#define COUNTER_CLOCK_TICK_RATE		(66666000)

/* cycles to nsec conversions taken from arch/arm/mach-omap1/time.c,
 * convert from cycles(64bits) => nanoseconds (64bits)
 */
#define CYC2NS_SCALE (((NSEC_PER_SEC / TICK_RATE_SCALE_FACTOR) << CYC2NS_SCALE_FACTOR) \
                      / (COUNTER_CLOCK_TICK_RATE / TICK_RATE_SCALE_FACTOR))

static inline unsigned long long notrace cycles_2_ns(unsigned long long cyc)
{
#if (CYC2NS_SCALE & 0x1)
	return (cyc * CYC2NS_SCALE << 1) >> (CYC2NS_SCALE_FACTOR + 1);
#else
	return (cyc * CYC2NS_SCALE) >> CYC2NS_SCALE_FACTOR;
#endif
}

unsigned long long notrace sched_clock(void)
{
	unsigned long long cyc;
	cyc = cnt32_to_63(readl(NE1xB_REFCOUNTER));
	return cycles_2_ns(cyc);
}

/* ---- NE1-TB UART ---- */
static struct plat_serial8250_port ne1xb_serial_ports[] = {
	[0] = {
		.membase	= (char *)IO_ADDRESS(NE1_BASE_UART_0),
		.mapbase	= (unsigned long)NE1_BASE_UART_0,
		.irq		= IRQ_UART_0,
		.flags		= UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 66666000*2,
	},
	[1] = {
		.membase	= (char *)IO_ADDRESS(NE1_BASE_UART_1),
		.mapbase	= (unsigned long)NE1_BASE_UART_1,
		.irq		= IRQ_UART_1,
		.flags		= UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 66666000*2,
	},
	[2] = {
		.membase	= (char *)IO_ADDRESS(NE1_BASE_UART_2),
		.mapbase	= (unsigned long)NE1_BASE_UART_2,
		.irq		= IRQ_UART_2,
		.flags		= UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 66666000*2,
	},
	[3] = {
		.flags		= 0,
	},
};

static struct platform_device ne1xb_serial_device = {
	.name           = "serial8250",
	.id             = 0,
	.dev            = {
		.platform_data  = ne1xb_serial_ports,
	},
	.num_resources  = 0,
	.resource       = NULL,
};


/* ---- NE1-TB NOR Flash ROM ---- */
static struct resource ne1xb_flash_resource[] = {
	[0] = {
		.start		= NE1TB_FLASH_BASE,
		.end		= NE1TB_FLASH_BASE + NE1TB_FLASH_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
};

static struct mtd_partition ne1xb_flash_partitions[] = {
	[0] = {
		.name		= "Bootloader",
		.size		= 0x00200000,
		.offset		= 0,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	[1] = {
		.name		= "kernel",
		.size		= 0x00400000,
		.offset		= MTDPART_OFS_APPEND,
		.mask_flags	= MTD_WRITEABLE	/* force read-only */
	},
	[2] = {
		.name		= "root",
		.size		= 0x03200000,
		.offset		= MTDPART_OFS_APPEND,
		.mask_flags	= MTD_WRITEABLE	/* force read-only */
	},
	[3] = {
		.name		= "home",
		.size		= (NE1TB_FLASH_SIZE - 0x00200000 - 0x00400000 - 0x03200000),
		.offset		= MTDPART_OFS_APPEND,
		.mask_flags	= 0,
	},
};

static struct flash_platform_data ne1xb_flash_data = {
	.map_name	= "cfi_probe",
	.width		= 2,
	.parts		= ne1xb_flash_partitions,
	.nr_parts	= ARRAY_SIZE(ne1xb_flash_partitions),
};

static struct platform_device ne1xb_flash_device = {
	.name		= "ne1xb-flash",
	.id		= 0,
	.dev		= {
		.platform_data	= &ne1xb_flash_data,
	},
	.num_resources	= ARRAY_SIZE(ne1xb_flash_resource),
	.resource	= &ne1xb_flash_resource[0],
};

/* ---- NE1-TB NAND Flash ROM ---- */
static struct mtd_partition ne1xb_nand_partition[] = {
	[0] = {
		.name   = "nand boot",
		.size   = 0x00060000,	/* 128K x 3 */
		.offset = 0,
	},
	[1] = {
		.name	= "nand filesystem",
		.size	= NE1TB_NAND_SIZE - 0x00060000,
		.offset	= MTDPART_OFS_APPEND,
	},
};
static struct platform_nand_chip ne1xb_nand_data = {
	.nr_chips      = 1,
	.chip_delay    = 15,
	.options       = 0,
	.partitions    = ne1xb_nand_partition,
	.nr_partitions = ARRAY_SIZE(ne1xb_nand_partition),
};
static struct resource ne1xb_nand_resource[] = {
	[0] = {
		.start = NE1_BASE_NAND_REG,
		.end   = NE1_BASE_NAND_REG + SZ_4K -1,
		.flags = IORESOURCE_MEM,
	},
};
static struct platform_device ne1xb_nand_device = {
	.name = "ne1xb-nand",
	.id   = -1,
	.dev  = {
		.platform_data = &ne1xb_nand_data,
		},
	.num_resources = ARRAY_SIZE(ne1xb_nand_resource),
	.resource = ne1xb_nand_resource,
};

#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)
static struct smsc911x_platform_config ne1xb_eth_config = {
	.flags		= SMSC911X_USE_32BIT,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
};
#else
/* ---- NE1-TB LAN9118 NIC ---- */
static struct smc911x_platdata ne1xb_eth_config = {
	.flags          = SMC911X_USE_32BIT,
	.irq_polarity	= 0,  /* Active Low */
	.irq_flags	= IRQF_SHARED,
};
#endif

static struct resource ne1xb_eth_resources[] = {
	[0] = {
		.start		= NE1xB_ETH_BASE,
		.end		= NE1xB_ETH_BASE + SZ_64K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_ETH,
		.end		= IRQ_ETH,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device ne1xb_eth_device = {
#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)
	.name		= "smsc911x",
#else
	.name		= "smc911x",
#endif
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ne1xb_eth_resources),
	.resource	= ne1xb_eth_resources,
	.dev = {
		.platform_data = &ne1xb_eth_config,
	},

};


/* ---- NE1 DMA ---- */
static struct platform_device ne1xb_dma_device = {
	.name = "dma",
	.id = -1,
};

/* ---- NE1 CSI ---- */
static struct platform_device ne1xb_csi0_device = {
	.name = "csi",
	.id = 0,
};
static struct platform_device ne1xb_csi1_device = {
	.name = "csi",
	.id = 1,
};

/* ---- NE1-TB MMC ---- */
static struct resource ne1xb_mmc_resources[] = {
	{
		.start = IRQ_MMC,
		.end   = IRQ_MMC,
		.flags = IORESOURCE_IRQ,
	},
};
static struct platform_device ne1xb_mmc_device = {
	.name = "ne1_mmc",
	.id = -1,
	.num_resources = ARRAY_SIZE(ne1xb_mmc_resources),
	.resource = ne1xb_mmc_resources,
};

/* ---- NE1-TB RTC ---- */
static struct resource ne1xb_rtc_resources[] = {
	{
		.start = IRQ_RTC,
		.end   = IRQ_RTC,
		.flags = IORESOURCE_IRQ,
	},
};
static struct platform_device ne1xb_rtc_device = {
	.name = "ne1xb_rtc",
	.id = -1,
	.num_resources = ARRAY_SIZE(ne1xb_rtc_resources),
	.resource = ne1xb_rtc_resources,
};

#ifdef CONFIG_NE1_USB
static u64 ne1_dmamask = ~(u32)0;
static void usb_release(struct device *dev)
{
}

/* ---- NE1-TB USB ---- */
static struct resource ne1xb_ohci_resources[] = {
	[0] = {
		.start		= NE1_BASE_USBH,
		.end		= NE1_BASE_USBH + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_OHCI,
		.end		= IRQ_OHCI,
		.flags		= IORESOURCE_IRQ,
	},
};
struct platform_device ne1xb_ohci_device = {
	.name = "ne1xb_ohci",
	.id = 0,
	.dev = {
		.release = usb_release,
		.dma_mask = &ne1_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ne1xb_ohci_resources),
	.resource	= ne1xb_ohci_resources,
};

static struct resource ne1xb_ehci_resources[] = {
	[0] = {
		.start		= NE1_BASE_USBH + 0x1000,
		.end		= NE1_BASE_USBH + 0x1000 + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_EHCI,
		.end		= IRQ_EHCI,
		.flags		= IORESOURCE_IRQ,
	},
};
struct platform_device ne1xb_ehci_device = {
	.name = "ne1xb_ehci",
	.id = -1,
	.dev = {
		.release = usb_release,
		.dma_mask = &ne1_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ne1xb_ehci_resources),
	.resource	= ne1xb_ehci_resources,
};
#endif


struct platform_device *platform_devs[] __initdata = {
	&ne1xb_dma_device,
	&ne1xb_csi0_device,
	&ne1xb_csi1_device,
	&ne1xb_mmc_device,
#ifdef CONFIG_NE1_USB
	&ne1xb_ohci_device,
	&ne1xb_ehci_device,
#endif
	&ne1xb_serial_device,
	&ne1xb_eth_device,
	&ne1xb_flash_device,
	&ne1xb_nand_device,

	&ne1xb_rtc_device,
};

void __init ne1_core_init(void)
{
	platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));
}

#ifdef CONFIG_LEDS
#define VA_LEDS_BASE (IO_ADDRESS(NE1TB_BASE_FPGA) + FPGA_FLED)

void ne1tb_leds_event(led_event_t ledevt)
{
	unsigned long flags;
	u32 val;

	local_irq_save(flags);
	val = readw(VA_LEDS_BASE);

	switch (ledevt) {
	case led_idle_start:
		val = val & ~FLED_LED0;
		break;

	case led_idle_end:
		val = val | FLED_LED0;
		break;

	case led_timer:
		val = val ^ FLED_LED1;
		break;

	case led_halted:
		val = 0;
		break;

	default:
		break;
	}

	writew(val, VA_LEDS_BASE);
	local_irq_restore(flags);
}
#endif	/* CONFIG_LEDS */

/*
 * Where is the timer (VA)?
 */
void __iomem *timer0_va_base;
void __iomem *timer1_va_base;
void __iomem *timer2_va_base;
void __iomem *timer3_va_base;
void __iomem *timer4_va_base;
void __iomem *timer5_va_base;

/*
 * How long is the timer interval?
 */
#define	TIMER_PCLK	(66666000)			/* 66.666 MHz */
#define TIMER_RELOAD	((TIMER_PCLK / 100) - 1)	/* ticks per 10 msec */
#define TIMER_PSCALE	(0x00000001)
#define TICKS2USECS(x)	((1000 * (x)) / ((TIMER_RELOAD + 1) / 10))

static void timer_set_mode(enum clock_event_mode mode,
			   struct clock_event_device *clk)
{
	switch(mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		writel(TIMER_RELOAD, timer0_va_base + TMR_TMRRST);
		writel(TMRCTRL_CE | TMRCTRL_CAE, timer0_va_base + TMR_TMRCTRL);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		/* timer enabled in 'next_event' hook */
		writel(TMRCTRL_CAE, timer0_va_base + TMR_TMRCTRL);
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		writel(0, timer0_va_base + TMR_TMRCTRL);
	}
}

static int timer_set_next_event(unsigned long evt,
				struct clock_event_device *unused)
{
	writel(evt, timer0_va_base + TMR_TMRRST);

	/* enable timer */
	writel(TMRCTRL_CE | TMRCTRL_CAE, timer0_va_base + TMR_TMRCTRL);

	return 0;
}

/*
 * Currently CLOCK_EVT_FEAT_ONESHOT feature is NOT supported
 * because timer0 is a periodic timer. We enable this feature here
 * by simulating an ONESHOT timer for experiments.
 */
static struct clock_event_device timer0_clockevent =	 {
	.name		= "timer0",
	.shift		= 32,
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode	= timer_set_mode,
	.set_next_event	= timer_set_next_event,
	.rating		= 300,
	.irq		= IRQ_TIMER_0,
	.cpumask	= cpu_all_mask,
};

static void __init ne1_clockevents_init(void)
{
	timer0_clockevent.mult =
		div_sc(TIMER_PCLK, NSEC_PER_SEC, timer0_clockevent.shift);
	timer0_clockevent.max_delta_ns =
		clockevent_delta2ns(0xffffffff, &timer0_clockevent);
	timer0_clockevent.min_delta_ns =
		clockevent_delta2ns(0x400, &timer0_clockevent);

	clockevents_register_device(&timer0_clockevent);
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t ne1_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = &timer0_clockevent;

	/* clear the interrupt */
	writel(GTINT_TCI, timer0_va_base + TMR_GTINT);

#ifdef CONFIG_TICK_ONESHOT
	/* Timer0 is and periodic timer.
	 * Hode up the count value to simulate an ONESHOT timer.
	 * It will be enabled later.
	 *   Hode: CE=1  CAE=0
	 */
	writel(TMRCTRL_CE, timer0_va_base + TMR_TMRCTRL);
#endif

	evt->event_handler(evt);

#ifdef CONFIG_TICK_ONESHOT
	/* enable timer again */
	writel(TMRCTRL_CE | TMRCTRL_CAE, timer0_va_base + TMR_TMRCTRL);
#endif

	return IRQ_HANDLED;
}

static struct irqaction ne1_timer_irq = {
	.name		= "timer tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= ne1_timer_interrupt,
};

static cycle_t ne1_get_cycles(struct clocksource *cs)
{
	return readl(timer1_va_base + TMR_TMRCNT);
}

static struct clocksource clocksource_ne1 = {
	.name	= "timer1",
	.rating	= 200,
	.read	= ne1_get_cycles,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 20,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __init ne1_clocksource_init(void)
{
	/* setup timer 1 as free-running clocksource */
	writel(TMRCTRL_CE, timer1_va_base + TMR_TMRCTRL); /* Hold */
	writel(0xffffffff, timer1_va_base + TMR_TMRRST);
	writel(TMRCTRL_CE | TMRCTRL_CAE, timer1_va_base + TMR_TMRCTRL);

	clocksource_ne1.mult =
		clocksource_hz2mult(TIMER_PCLK, clocksource_ne1.shift);
	clocksource_register(&clocksource_ne1);
}

/*
 * Set up the clock source and clock events devices
 */
static void __init ne1_timer_init(void)
{
	timer0_va_base = __io_address(NE1_BASE_TIMER_0);
	timer1_va_base = __io_address(NE1_BASE_TIMER_1);
	timer2_va_base = __io_address(NE1_BASE_TIMER_2);
	timer3_va_base = __io_address(NE1_BASE_TIMER_3);
	timer4_va_base = __io_address(NE1_BASE_TIMER_4);
	timer5_va_base = __io_address(NE1_BASE_TIMER_5);

	/*
	 * Initialise to a known state (all timers off)
	 */
	writel(0, timer0_va_base + TMR_TMRCTRL);	/* CE=0, CAE=0 */
	writel(TMRCTRL_CE, timer1_va_base + TMR_TMRCTRL); /* Hold */
	writel(0, timer2_va_base + TMR_TMRCTRL);
	writel(0, timer3_va_base + TMR_TMRCTRL);
	writel(0, timer4_va_base + TMR_TMRCTRL);
	writel(0, timer5_va_base + TMR_TMRCTRL);
	writel(0, timer0_va_base + TMR_GTICTRL);	/* RIS_C=FAL_C=0 */
	writel(0, timer1_va_base + TMR_GTICTRL);
	writel(0, timer2_va_base + TMR_GTICTRL);
	writel(0, timer3_va_base + TMR_GTICTRL);
	writel(0, timer4_va_base + TMR_GTICTRL);
	writel(0, timer5_va_base + TMR_GTICTRL);

#ifdef CONFIG_LOCAL_TIMERS
	twd_base = __io_address(NE1_TWD_BASE);
#endif

	/*
	 * Set pre-scaler.
	 *   Input PCLK is 66.666 MHz
	 *   PSCAL=0b000001(1/1), PSCALE2=0x00
	 *   Timer precision is 15 ns
	 */
	writel(TIMER_PSCALE, timer0_va_base + TMR_PSCALE);
	writel(TIMER_PSCALE, timer1_va_base + TMR_PSCALE);

	/*
	 * Set reset register, clear all pending interrupts,
	 * enable timer interrupt (TCE=1), and start timer.
	 */
	writel(0xffffffff, timer0_va_base + TMR_GTINT);
	writel(GTINTEN_TCE, timer0_va_base + TMR_GTINTEN);

	/*
	 * Make irqs happen for the system timer, and start
	 */
	setup_irq(IRQ_TIMER_0, &ne1_timer_irq);

	ne1_clocksource_init();
	ne1_clockevents_init();
}

struct sys_timer ne1_timer = {
	.init		= ne1_timer_init,
};

