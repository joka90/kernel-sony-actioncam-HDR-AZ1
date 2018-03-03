/* 2013-01-04: File added and changed by Sony Corporation */
/*
 *  linux/drivers/input/touchscreen/kzm9d_ts.c
 *
 *  Copyright (C) 2010 Kyoto Micro Computer Co.,LTD.
 *
 *  This file is based on a9tc_ts.c
 *
 *  Copyright (C) 2008 NEC Electronics Corporation 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <asm/delay.h>

#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/spi.h>

#define KZM9D_TS_NAME	"kzm9d_touch"

#define KZM9D_TS_DEF_SCAN_TIME	20	// msec

#define	MAX_12BIT	((1 << 12) - 1)

#define LANDSCAPE_SCREEN

#ifdef CONFIG_EMXX_ANDROID
#define X_MIN	150
#define X_MAX	4000
#define Y_MIN	300
#define Y_MAX	3600
#else
#define X_MIN	0
#define X_MAX	MAX_12BIT
#define Y_MIN	0
#define Y_MAX	MAX_12BIT
#endif


static struct input_dev *inpdev = NULL;

static int kzm9d_ts_irq = INT_PAD;

static void kzm9d_ts_timer(unsigned long data);
static struct timer_list kzm9d_ts_scan_timer = {
	.function = kzm9d_ts_timer,
	.data = 0,
};

static u32 scan_interval = KZM9D_TS_DEF_SCAN_TIME;
static u32 debug = 0;
//static u32 debug = 1;

static inline int is_pen_down(void)
{
	return gpio_get_value(GPIO_PAD)? 0 : 1;
}


static SPI_CONFIG device0_config = {
	.dev	= SPI_DEV_SP0,
	.cs_sel	= SPI_CS_SEL_CS1,
	.m_s	= SPI_M_S_MASTER,
	.dma	= SPI_DMA_OFF,
	.pol	= SPI_POL_SP0_CS1,
	.tiecs	= SPI_TIECS_FIXED,
	.nbw	= SPI_NB_8BIT,
	.nbr	= SPI_NB_16BIT,
	.sclk	= SPI_SCLK_1500KHZ,
};

/*
 * TI ADS7843 touch screen contoller
 */

static void read_xy(u16 *x, u16*y)
{
	u16 x_point, y_point;
	char cmd;
	int ret;

	cmd = 0xd0;
	if ((ret = spi_cmd_read(&device0_config, &cmd, (char*)&x_point, SPI_RW_2CYCLE)) < 0) {

		printk("spi_cmd_read failed. %d ", ret);
	}
	cmd = 0x90;
	if ((ret = spi_cmd_read(&device0_config, &cmd, (char*)&y_point, SPI_RW_2CYCLE)) < 0) {

		printk("spi_cmd_read failed. %d ", ret);
	}

#if 0
	printk("x_point = %x\n", x_point);
	printk("y_point = %x\n", y_point);
#endif

	*x = x_point >> 3;
	*y = y_point >> 3;
}

static void kzm9d_ts_timer(unsigned long data)
{
	u16 x = 0;
	u16 y = 0;
	u8 pen_down = 0;
	static u8 prev_pen_down = 0;

	if (is_pen_down()) {
		read_xy(&x, &y);

		/* double check */
		if (is_pen_down()) {
			pen_down = 1;
		}
	}

#ifdef CONFIG_EMXX_ANDROID
#ifdef LANDSCAPE_SCREEN
	{
		int x2 = (X_MIN + X_MAX) - x;
		if (x2 < 0) {
			x = X_MIN;
		} else {
			x = x2;
		}
	}
#else
	{
		int y2 = (Y_MIN + Y_MAX) - y;
		if (y2 < 0) {
			y = Y_MIN;
		} else {
			y = y2;
		}
	}
#endif
#endif

	if (pen_down) {
		if (prev_pen_down == 0) {
			input_report_key(inpdev, BTN_TOUCH, 1);
		}
		input_report_abs(inpdev, ABS_X, x);
		input_report_abs(inpdev, ABS_Y, y);
		input_report_abs(inpdev, ABS_PRESSURE, 50);
		input_sync(inpdev);

		mod_timer(&kzm9d_ts_scan_timer, jiffies + msecs_to_jiffies(scan_interval));
	} else {
		if (prev_pen_down) {
			input_report_key(inpdev, BTN_TOUCH, 0);
			input_report_abs(inpdev, ABS_PRESSURE, 0);
			input_sync(inpdev);
		}
		enable_irq(kzm9d_ts_irq);
	}

	if (debug) {
		printk("%s: (%d:%d), (%x:%x)\n", (pen_down == 0) ? "UP":"DOWN" ,x ,y, x, y);
	}

	prev_pen_down = pen_down;
}

static irqreturn_t kzm9d_ts_handler(int irq, void *dev_id)
{
	if (debug) {
		printk("%s: irq handler called\n", __FUNCTION__);
	}

	disable_irq_nosync(kzm9d_ts_irq);
	mod_timer(&kzm9d_ts_scan_timer, jiffies + msecs_to_jiffies(scan_interval));

	return IRQ_HANDLED;
}

static int kzm9d_ts_probe(struct platform_device *dev)
{
	int ret;

	inpdev = input_allocate_device();
	if (!inpdev) {
		printk("%s: error allocating memory for input structure\n", __FUNCTION__);
		return -ENOMEM;
	}

	init_timer(&kzm9d_ts_scan_timer);

	/* request irq */
	irq_set_irq_type(kzm9d_ts_irq, IRQ_TYPE_EDGE_FALLING);
	if (request_irq(kzm9d_ts_irq, kzm9d_ts_handler, IRQF_DISABLED, KZM9D_TS_NAME, NULL) < 0) {
		printk("%s: failed to request_irq\n", __FUNCTION__);
		input_free_device(inpdev);
		return -EINVAL;
	}

	inpdev->name = KZM9D_TS_NAME;
	inpdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	inpdev->keybit[BIT_WORD(BTN_TOUCH)] |= BIT_MASK(BTN_TOUCH);

	/* value based on board measurement */
	input_set_abs_params(inpdev, ABS_X, X_MIN, X_MAX, 0, 0);
	input_set_abs_params(inpdev, ABS_Y, Y_MIN, Y_MAX, 0, 0);
	input_set_abs_params(inpdev, ABS_PRESSURE, 0, 100, 0, 0);

	ret = input_register_device(inpdev);
	if (ret != 0) {
		printk("%s: error registering with input system\n", __FUNCTION__);
		free_irq(kzm9d_ts_irq, NULL);
		input_free_device(inpdev);
		return ret;
	}

	return 0;
}

static int kzm9d_ts_remove(struct platform_device *pdev)
{
	input_unregister_device(inpdev);
	free_irq(kzm9d_ts_irq, NULL);

	return 0;
}

static struct platform_driver kzm9d_ts_driver = {
	.probe 		= kzm9d_ts_probe,
	.remove 	= kzm9d_ts_remove,
	.driver = {
		.name	= KZM9D_TS_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init kzm9d_ts_init(void)
{
	return platform_driver_register(&kzm9d_ts_driver);
}

static void __exit kzm9d_ts_exit(void)
{
	platform_driver_unregister(&kzm9d_ts_driver);
}

module_init(kzm9d_ts_init);
module_exit(kzm9d_ts_exit);

module_param(scan_interval, uint, 0644);
MODULE_PARM_DESC(scan_interval, "scan interval time [ms]");

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Enable(1)/Disable(0) debug message");

MODULE_AUTHOR("KMC");
MODULE_LICENSE("GPL");
