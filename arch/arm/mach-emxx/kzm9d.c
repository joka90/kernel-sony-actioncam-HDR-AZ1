/* 2011-06-06: File added and changed by Sony Corporation */
/*
 *  File Name		: arch/arm/mach-emxx/kzm9d.c
 *  Function		: kzm9d
 *
 * Copyright (C) 2010 Renesas Electronics Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/smsc911x.h>
#ifdef CONFIG_EMXX_ANDROID
#include <linux/android_pmem.h>
#endif

#include <asm/pmu.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/cache-l2x0.h>

#include <mach/hardware.h>
#include <mach/smu.h>
#include <mach/emxx_mem.h>
#include <mach/irqs.h>

#include "generic.h"
#include "timer.h"

static int __initdata emxx_serial_ports[] = { 1, 1, 0, 0 };

static struct resource pmu_resources[] = {
	[0] = {
		.start		= INT_PMONI0,
		.end		= INT_PMONI0,
		.flags		= IORESOURCE_IRQ,
	},
	[1] = {
		.start		= INT_PMONI1,
		.end		= INT_PMONI1,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device pmu_device = {
        .name                   = "arm-pmu",
        .id                     = ARM_PMU_DEVICE_CPU,
        .num_resources          = ARRAY_SIZE(pmu_resources),
        .resource               = pmu_resources,
};

/* Ether */
static struct resource smsc911x_resources[] = {
	[0] = {
		.start	= EMEV_ETHER_BASE,
		.end	= EMEV_ETHER_BASE + SZ_64K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INT_ETHER,
		.end	= INT_ETHER,
		.flags	= IORESOURCE_IRQ,
	},
};
static struct smsc911x_platform_config smsc911x_platdata = {
	.flags		= SMSC911X_USE_32BIT,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_HIGH,
};
static struct platform_device smc91x_device = {
	.name	= "smsc911x",
	.id	= 0,
	.dev	= {
		  .platform_data = &smsc911x_platdata,
		},
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
};

/* Keypad */
static struct platform_device kzm9d_key_device = {
	.name	= "kzm9d_key",
	.id	= -1,
};

/* Touch Panel */
static struct platform_device kzm9d_ts_device = {
	.name	= "kzm9d_touch",
	.id	= -1,
};

/* Light */
static struct platform_device emxx_light_device = {
	.name	= "emxx-light",
	.id	= -1,
};

/* Battery */
static struct platform_device emxx_battery_device = {
	.name	= "emxx-battery",
	.id	= -1,
};

#ifdef CONFIG_EMXX_ANDROID
/* PMEM */
static struct android_pmem_platform_data android_pmem_pdata = {
	.name	= "pmem",
	.start	= EMXX_PMEM_BASE,
	.size	= EMXX_PMEM_SIZE,
	.no_allocator = 1,
	.cached	= 1,
};
static struct platform_device android_pmem_device = {
	.name	= "android_pmem",
	.id	= 0,
	.dev	= {
		  .platform_data = &android_pmem_pdata
		},
};
#endif

static struct platform_device *devs[] __initdata = {
	&pmu_device,
	&smc91x_device,
	&kzm9d_ts_device,
	&kzm9d_key_device,
	&emxx_light_device,
	&emxx_battery_device,
#ifdef CONFIG_EMXX_ANDROID
	&android_pmem_device,
#endif
};


static struct i2c_board_info emev_i2c_devices[] = {
	{
	  I2C_BOARD_INFO(I2C_SLAVE_CODEC_NAME,  I2C_SLAVE_CODEC_ADDR),
	},
	{
	  I2C_BOARD_INFO(I2C_SLAVE_RTC_NAME,    I2C_SLAVE_RTC_ADDR),
	},
};

static void __init kzm9d_map_io(void)
{
	emxx_map_io();
	system_rev = readl(EMXX_SRAM_VIRT + 0x1ffe0);
}

static void __init emev_init_irq(void)
{
	/* core tile GIC, primary */
	gic_init(0, INT_CPU_TIM, __io_address(EMXX_INTA_DIST_BASE), __io_address(EMXX_INTA_CPU_BASE));
	gic_emxx_init(0, INT_CPU_TIM);

}

static void __init kzm9d_init(void)
{
	emxx_serial_init(emxx_serial_ports);

#ifdef CONFIG_SMP
	writel(0xfff00000, EMXX_SCU_VIRT + 0x44);
	writel(0xffe00000, EMXX_SCU_VIRT + 0x40);
	writel(0x00000003, EMXX_SCU_VIRT + 0x00);
#endif

#ifdef CONFIG_EMXX_L310
	{
		void __iomem *l2cc_base = (void *)EMXX_L2CC_VIRT;
#ifdef CONFIG_EMXX_L310_WT
		/* Force L2 write through */
		writel(0x2, l2cc_base + L2X0_DEBUG_CTRL);
#endif
		writel(0x111, l2cc_base + L2X0_TAG_LATENCY_CTRL);
		writel(0x111, l2cc_base + L2X0_DATA_LATENCY_CTRL);
#ifdef CONFIG_SMP
		writel(0xfff00000, l2cc_base + 0xc04);
		writel(0xffe00001, l2cc_base + 0xc00);
#endif

#ifdef CONFIG_EMXX_L310_8WAY
		/* 8-way 32KB cache, Early BRESP */
		writel(0, SMU_CPU_ASSOCIATIVITY);	/* 0:8-way 1:16-way */
		writel(2, SMU_CPU_WAYSIZE);		/* 0,1:16KB 2:32KB */
		l2x0_init(l2cc_base, 0x40040000, 0x00000fff);
#else
		/* 16-way 16KB cache, Early BRESP */
		writel(1, SMU_CPU_ASSOCIATIVITY);	/* 0:8-way 1:16-way */
		writel(1, SMU_CPU_WAYSIZE);		/* 0,1:16KB 2:32KB */
		l2x0_init(l2cc_base, 0x40030000, 0x00000fff);
#endif
	}
#endif	/* CONFIG_EMXX_L310 */

	platform_add_devices(devs, ARRAY_SIZE(devs));
	i2c_register_board_info(0, emev_i2c_devices,
			ARRAY_SIZE(emev_i2c_devices));

	printk(KERN_INFO "chip revision %x\n", system_rev);

#ifdef CONFIG_EMXX_QR
#ifdef CONFIG_SMP
	if ((system_rev & EMXX_REV_MASK) == EMXX_REV_ES1)
		return;
#endif /* CONFIG_SMP */
	if ((system_rev & EMXX_REV_MASK) == EMXX_REV_ES1)
		writel(0x01111101, SMU_PC_SWENA);
	writel(0x00444444, SMU_QR_WAITCNT);
	writel(0x00000003, SMU_QR_WFI);
#endif /* CONFIG_EMXX_QR */
}

MACHINE_START(KZM9D, "KZM9D")
	.boot_params  = PHYS_OFFSET + 0x100,
	.soft_reboot  = 1,
	.map_io       = kzm9d_map_io,
	.init_irq     = emev_init_irq,
	.init_machine = kzm9d_init,
	.timer        = &emxx_timer,
MACHINE_END
