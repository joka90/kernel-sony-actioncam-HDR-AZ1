/* 2013-08-20: File changed by Sony Corporation */
/*
 * linux/arch/arm/mach-omap2/board-omap3logic.c
 *
 * Copyright (C) 2010 Li-Pro.Net
 * Stephan Linz <linz@li-pro.net>
 *
 * Copyright (C) 2010 Logic Product Development, Inc.
 * Peter Barada <peter.barada@logicpd.com>
 *
 * Modified from Beagle, EVM, and RX51
 *
 * Copyright (C) 2011 Meprolight, Ltd.
 * Alex Gershgorin <alexg@xxxxxxxxxxxxxx>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>

#include <linux/regulator/machine.h>

#include <linux/i2c/tsc2004.h>
#include <linux/i2c/twl.h>
#include <linux/mmc/host.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include "mux.h"
#include "hsmmc.h"
#include "timer-gp.h"
#include "control.h"
#include "common-board-devices.h"

#include <plat/mux.h>
#include <plat/board.h>
#include <plat/common.h>
#include <plat/gpmc-smsc911x.h>
#include <plat/gpmc.h>
#include <plat/sdrc.h>
#include <plat/nand.h>
#include <plat/vram.h>

#include <video/omapdss.h>
#include <video/omap-panel-generic-dpi.h>

#define OMAP3LOGIC_SMSC911X_CS			1

#define OMAP3530_LV_SOM_MMC_GPIO_CD		110
#define OMAP3530_LV_SOM_MMC_GPIO_WP		126
#define OMAP3530_LV_SOM_SMSC911X_GPIO_IRQ	152

#define OMAP3_TORPEDO_MMC_GPIO_CD		127
#define OMAP3_TORPEDO_SMSC911X_GPIO_IRQ		129


static struct mtd_partition omap3torpedo_nand_partitions[] = {
	{
		.name           = "NoLo-NAND",
		.offset         = 0,
		.size           = 1 * NAND_BLOCK_SIZE,
		.mask_flags     = MTD_WRITEABLE,
	}, {
		.name           = "Lboot-NAND",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 17 * NAND_BLOCK_SIZE,
		.mask_flags     = MTD_WRITEABLE,
	}, {
		.name           = "Linux Kernel",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 100 * NAND_BLOCK_SIZE,
	}, {
		.name           = "Linux Filesystem",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 1928 * NAND_BLOCK_SIZE,
	}, {
		.name           = "u-boot Env-NAND",
		.offset         = MTDPART_OFS_APPEND,
		.size           = MTDPART_SIZ_FULL,
		.mask_flags     = MTD_WRITEABLE,
	},
};

static struct omap_nand_platform_data torpedo_nand_data = {
	.cs             = 0,
	.devsize        = NAND_BUSWIDTH_16,
	.parts          = omap3torpedo_nand_partitions,
	.nr_parts       = ARRAY_SIZE(omap3torpedo_nand_partitions),
};

static struct regulator_consumer_supply omap3logic_vmmc1_supply = {
	.supply			= "vmmc",
};

static struct regulator_consumer_supply omap3logic_vdds_supplies[] = {
	REGULATOR_SUPPLY("vdds_sdi", "omapdss"),
	REGULATOR_SUPPLY("vdds_dsi", "omapdss"),
};

static struct regulator_consumer_supply omap3logic_vsim_supply = {
	.supply                 = "vmmc_aux",
};

static struct regulator_consumer_supply omap3logic_vdac_supply =
	REGULATOR_SUPPLY("vdda_dac", "omapdss_venc");

/* VMMC1 for MMC1 pins CMD, CLK, DAT0..DAT3 (20 mA, plus card == max 220 mA) */
static struct regulator_init_data omap3logic_vmmc1 = {
	.constraints = {
		.name			= "VMMC1",
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &omap3logic_vmmc1_supply,
};

static struct regulator_init_data omap3logic_vpll2 = {
	.constraints = {
		.name                   = "VDDS_DSI",
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.apply_uV               = true,
		.always_on              = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL	| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE,
	},
	.num_consumer_supplies  = ARRAY_SIZE(omap3logic_vdds_supplies),
	.consumer_supplies      = omap3logic_vdds_supplies,
};

static struct regulator_init_data omap3logic_vdac = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL	| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &omap3logic_vdac_supply,
};

static struct regulator_init_data omap3logic_vsim = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 3000000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &omap3logic_vsim_supply,
};

static struct twl4030_gpio_platform_data omap3logic_gpio_data = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.use_leds	= true,
	.pullups	= BIT(1),
	.pulldowns	= BIT(2)  | BIT(6)  | BIT(7)  | BIT(8)
			| BIT(13) | BIT(15) | BIT(16) | BIT(17),
};

static struct twl4030_platform_data omap3logic_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,

	/* platform_data for children goes here */
	.gpio		= &omap3logic_gpio_data,
	.vmmc1		= &omap3logic_vmmc1,
	.vsim           = &omap3logic_vsim,
	.vpll2          = &omap3logic_vpll2,
	.vdac           = &omap3logic_vdac,
};

/*
 * TSC 2004 Support
 */
#define	GPIO_TSC2004_IRQ	153

static int tsc2004_init_irq(void)
{
	int ret = 0;

	omap_mux_init_gpio(GPIO_TSC2004_IRQ, OMAP_PIN_INPUT);
	ret = gpio_request(GPIO_TSC2004_IRQ, "tsc2004-irq");
	if (ret < 0) {
		printk(KERN_WARNING "failed to request GPIO#%d: %d\n",
				GPIO_TSC2004_IRQ, ret);
		return ret;
	}

	if (gpio_direction_input(GPIO_TSC2004_IRQ)) {
		printk(KERN_WARNING "GPIO#%d cannot be configured as "
				"input\n", GPIO_TSC2004_IRQ);
		return -ENXIO;
	}

	gpio_set_debounce(GPIO_TSC2004_IRQ, 310);
	return ret;
};

static void tsc2004_exit_irq(void)
{
	gpio_free(GPIO_TSC2004_IRQ);
}

static int tsc2004_get_irq_level(void)
{
	return gpio_get_value(GPIO_TSC2004_IRQ) ? 0 : 1;
}

struct tsc2004_platform_data omap3logic_tsc2004data = {
	.model = 2004,
	.x_plate_ohms = 180,
	.get_pendown_state = tsc2004_get_irq_level,
	.init_platform_hw = tsc2004_init_irq,
	.exit_platform_hw = tsc2004_exit_irq,
};

static struct i2c_board_info __initdata omap3logic_i2c3_boardinfo[] = {
	{
		I2C_BOARD_INFO("tsc2004", 0x48),
		.type		= "tsc2004",
		.platform_data = &omap3logic_tsc2004data,
		.irq = OMAP_GPIO_IRQ(GPIO_TSC2004_IRQ),
	},
};

static int __init omap3logic_i2c_init(void)
{
	omap3_pmic_init("twl4030", &omap3logic_twldata);

	/* Initializing the i2c bus 2 and 3 */
	omap_register_i2c_bus(2, 400, NULL, 0);
	omap_register_i2c_bus(3, 400, omap3logic_i2c3_boardinfo,
				ARRAY_SIZE(omap3logic_i2c3_boardinfo));
	return 0;
}

static struct omap2_hsmmc_info __initdata board_mmc_info[] = {
	{
		.name		= "external",
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
	{}      /* Terminator */
};

static void __init board_mmc_init(void)
{
	if (machine_is_omap3530_lv_som()) {
		/* OMAP3530 LV SOM board */
		board_mmc_info[0].gpio_cd = OMAP3530_LV_SOM_MMC_GPIO_CD;
		board_mmc_info[0].gpio_wp = OMAP3530_LV_SOM_MMC_GPIO_WP;
		omap_mux_init_signal("gpio_110", OMAP_PIN_OUTPUT);
		omap_mux_init_signal("gpio_126", OMAP_PIN_OUTPUT);
	} else if (machine_is_omap3_torpedo()) {
		/* OMAP3 Torpedo board */
		board_mmc_info[0].gpio_cd = OMAP3_TORPEDO_MMC_GPIO_CD;
		omap_mux_init_signal("gpio_127", OMAP_PIN_OUTPUT);
	} else {
		/* unsupported board */
		printk(KERN_ERR "%s(): unknown machine type\n", __func__);
		return;
	}

	omap2_hsmmc_init(board_mmc_info);
	/* link regulators to MMC adapters */
	omap3logic_vmmc1_supply.dev = board_mmc_info[0].dev;
}

static struct omap_smsc911x_platform_data __initdata board_smsc911x_data = {
	.cs             = OMAP3LOGIC_SMSC911X_CS,
	.gpio_irq       = -EINVAL,
	.gpio_reset     = -EINVAL,
};

/* TODO/FIXME (comment by Peter Barada, LogicPD):
 * Fix the PBIAS voltage for Torpedo MMC1 pins that
 * are used for other needs (IRQs, etc).            */
static void omap3torpedo_fix_pbias_voltage(void)
{
	u16 control_pbias_offset = OMAP343X_CONTROL_PBIAS_LITE;
	u32 reg;

	if (machine_is_omap3_torpedo())
	{
		/* Set the bias for the pin */
		reg = omap_ctrl_readl(control_pbias_offset);

		reg &= ~OMAP343X_PBIASLITEPWRDNZ1;
		omap_ctrl_writel(reg, control_pbias_offset);

		/* 100ms delay required for PBIAS configuration */
		msleep(100);

		reg |= OMAP343X_PBIASLITEVMODE1;
		reg |= OMAP343X_PBIASLITEPWRDNZ1;
		omap_ctrl_writel(reg | 0x300, control_pbias_offset);
	}
}

static inline void __init board_smsc911x_init(void)
{
	if (machine_is_omap3530_lv_som()) {
		/* OMAP3530 LV SOM board */
		board_smsc911x_data.gpio_irq =
					OMAP3530_LV_SOM_SMSC911X_GPIO_IRQ;
		omap_mux_init_signal("gpio_152", OMAP_PIN_INPUT);
	} else if (machine_is_omap3_torpedo()) {
		/* OMAP3 Torpedo board */
		board_smsc911x_data.gpio_irq = OMAP3_TORPEDO_SMSC911X_GPIO_IRQ;
		omap_mux_init_signal("gpio_129", OMAP_PIN_INPUT);
	} else {
		/* unsupported board */
		printk(KERN_ERR "%s(): unknown machine type\n", __func__);
		return;
	}

	gpmc_smsc911x_init(&board_smsc911x_data);
}

static void __init omap3logic_init_early(void)
{
	omap2_init_common_infrastructure();
	omap2_init_common_devices(NULL, NULL);
}

#if defined(CONFIG_FB_OMAP2) || defined(CONFIG_FB_OMAP2_MODULE)

#define OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO        154
#define OMAP3_TORPEDO_LCD_ENABLE_GPIO           155
#define OMAP3_TORPEDO_LCD_PWM_GPIO              56

static int omap3logic_enable_lcd(struct omap_dss_device *dssdev)
{
	gpio_set_value(OMAP3_TORPEDO_LCD_ENABLE_GPIO, 1);
	msleep(20);
	gpio_set_value_cansleep(OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO, 1);
	return 0;
}

static void omap3logic_disable_lcd(struct omap_dss_device *dssdev)
{
	gpio_set_value(OMAP3_TORPEDO_LCD_ENABLE_GPIO, 0);
	msleep(20);
	gpio_set_value_cansleep(OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO, 0);
}

static struct panel_generic_dpi_data lcd_panel = {
	.name                   = "sharp_lq",
	.platform_enable        = omap3logic_enable_lcd,
	.platform_disable       = omap3logic_disable_lcd,
};

static struct omap_dss_device omap3logic_lcd_device = {
	.name                   = "lcd",
	.driver_name            = "generic_dpi_panel",
	.type                   = OMAP_DISPLAY_TYPE_DPI,
	.data                   = &lcd_panel,
	.phy.dpi.data_lines     = 16,
};

static struct omap_dss_device omap3logic_tv_device = {
	.name                   = "tv",
	.driver_name            = "venc",
	.type                   = OMAP_DISPLAY_TYPE_VENC,
	.phy.venc.type          = OMAP_DSS_VENC_TYPE_SVIDEO,
};

static struct omap_dss_device *omap3logic_dss_devices[] = {
	&omap3logic_lcd_device,
	&omap3logic_tv_device,
};

static struct omap_dss_board_info omap3logic_dss_data = {
	.num_devices            = ARRAY_SIZE(omap3logic_dss_devices),
	.devices                = omap3logic_dss_devices,
	.default_device         = &omap3logic_lcd_device,
};

static void __init omap3logic_display_init(void)
{
	int r;

	/* Requesting GPIO line for LCD Power */
	if (gpio_request(OMAP3_TORPEDO_LCD_PWM_GPIO, "LCD mdisp"))
		printk(KERN_ERR "Can't request GPIO %d for LCD mdisp\n", OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO);
	gpio_direction_output(OMAP3_TORPEDO_LCD_PWM_GPIO, 1);
	omap_mux_init_gpio(OMAP3_TORPEDO_LCD_PWM_GPIO, OMAP_PIN_OUTPUT);

	/* Requesting GPIO line for LCD Backlight*/
	if (gpio_request(OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO, "LCD backlight"))
		printk(KERN_ERR "Can't request GPIO %d for LCD backlight\n", OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO);
	gpio_direction_output(OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO, 0);
	omap_mux_init_gpio(OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO, OMAP_PIN_OUTPUT);

	/* Requesting GPIO line for LCD Enable*/
	if (gpio_request(OMAP3_TORPEDO_LCD_ENABLE_GPIO, "LCD enable"))
		printk(KERN_ERR "Can't request GPIO %d for LCD enable\n", OMAP3_TORPEDO_LCD_ENABLE_GPIO);
	gpio_direction_output(OMAP3_TORPEDO_LCD_ENABLE_GPIO, 1);
	omap_mux_init_gpio(OMAP3_TORPEDO_LCD_ENABLE_GPIO, OMAP_PIN_OUTPUT);

	/* Sleep for 600ms since hte 4.3" display needs
	* power before turning on the clocks */
	msleep(600);

	/* Bring up backlight */
	gpio_direction_output(OMAP3_TORPEDO_LCD_BACKLIGHT_GPIO, 1);

	r = omap_display_init(&omap3logic_dss_data);
	if (r) {
		printk(KERN_ERR "Failed to register DSS device!!\n");
	}

}

static void __init omap3logic_video_mem_init(void)
{
	omap_vram_set_sdram_vram(PAGE_ALIGN(480 * 272 * 4) +
		2 * PAGE_ALIGN(480 * 272 * 4 * 2), 0);
}

#else
static void __init omap3logic_display_init(void) { }
#endif /* defined(CONFIG_FB_OMAP2) || defined(CONFIG_FB_OMAP2_MODULE) */

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#endif

static void __init omap3logic_reserve(void)
{
	omap3logic_video_mem_init();
	omap_reserve();
}

static void __init omap3logic_init(void)
{
	omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);
	omap3torpedo_fix_pbias_voltage();
	omap3logic_i2c_init();
	omap_serial_init();
	board_mmc_init();
	board_smsc911x_init();
	gpmc_nand_init(&torpedo_nand_data);
	omap3logic_reserve();
	omap3logic_display_init();

	/* Ensure SDRC pins are mux'd for self-refresh */
	omap_mux_init_signal("sdrc_cke0", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("sdrc_cke1", OMAP_PIN_OUTPUT);
}

MACHINE_START(OMAP3_TORPEDO, "Logic OMAP3 Torpedo board")
	.boot_params	= 0x80000100,
	.map_io		= omap3_map_io,
	.init_early	= omap3logic_init_early,
	.init_irq	= omap_init_irq,
	.init_machine	= omap3logic_init,
	.timer		= &omap_timer,
MACHINE_END

MACHINE_START(OMAP3530_LV_SOM, "OMAP Logic 3530 LV SOM board")
	.boot_params	= 0x80000100,
	.map_io		= omap3_map_io,
	.init_early	= omap3logic_init_early,
	.init_irq	= omap_init_irq,
	.init_machine	= omap3logic_init,
	.timer		= &omap_timer,
MACHINE_END
