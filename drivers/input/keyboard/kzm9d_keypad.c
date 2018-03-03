/* 2011-01-25: File added and changed by Sony Corporation */
/*
 *  linux/drivers/input/touchscreen/kzm9d_keypad.c
 *
 *  Copyright (C) 2010 Kyoto Micro Computer Co.,LTD.
 *
 *  This file is based on a9tc_keypad.c
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
#include <linux/proc_fs.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/delay.h>

#include <mach/hardware.h>
#include <mach/gpio.h>

#define KZM9D_KP_NAME	"kzm9d_key"
#define KZM9D_KP_DEF_SCAN_TIME	100	// msec

#define KZM9D_KP_SCAN_LOOP_TIME	30	// msec


static struct input_dev *inpdev = NULL;

static void kzm9d_kp_timer(unsigned long data);
static struct timer_list kzm9d_kp_scan_timer = {
	.function = kzm9d_kp_timer,
	.data = 0,
};

static u32 scan_interval = KZM9D_KP_DEF_SCAN_TIME;
static u32 debug = 0;

static u8 kzm9d_kp_keycode[] = {
	KEY_UP, KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_ENTER, KEY_F1, KEY_BACK, KEY_HOME, KEY_SEARCH
};

static u8 *code_name[] = {
	"UP",  "RIGHT", "LEFT", "DOWN", "ENTER", "F1", "BACK", "HOME", "SEARCH"
};

#ifdef CONFIG_EMXX_ANDROID
#include <linux/proc_fs.h>
#include <linux/wakelock.h>
#include <linux/uaccess.h>

static struct wake_lock key_lock;

static int dummy_code[] = {
	KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
	KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, 
	KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, 
	KEY_Y, KEY_Z, 
	KEY_EQUAL, KEY_MINUS, KEY_TAB, KEY_SLASH, KEY_SPACE, KEY_KPPLUS, KEY_F1, KEY_BACKSPACE,
	KEY_DOT, KEY_VOLUMEDOWN, KEY_VOLUMEUP
};

static void repo_dummy(int code)
{
	input_report_key(inpdev, code, 1);
	schedule_timeout(1);
	input_report_key(inpdev, code, 0);
}

static int key_read_proc( char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
	return 0;
}

#define MAX_BUF 255

static int key_write_proc( struct file *file, const char *buf, u_long count, void *data )
{
	char mybuf[MAX_BUF];
	int i, ret;

	if (count >= MAX_BUF)
		return -EINVAL;

	ret = copy_from_user(mybuf, buf, count);

	for (i=0; i<count; i++) {
		if (mybuf[i] == '\n' || mybuf[i] == '\0') {
			break;
		}
		switch (mybuf[i]) {
		case '1':  repo_dummy(KEY_1); break;
		case '2':  repo_dummy(KEY_2); break;
		case '3':  repo_dummy(KEY_3); break;
		case '4':  repo_dummy(KEY_4); break;
		case '5':  repo_dummy(KEY_5); break;
		case '6':  repo_dummy(KEY_6); break;
		case '7':  repo_dummy(KEY_7); break;
		case '8':  repo_dummy(KEY_8); break;
		case '9':  repo_dummy(KEY_9); break;
		case '0':  repo_dummy(KEY_0); break;
		case 'a':  repo_dummy(KEY_A); break;
		case 'b':  repo_dummy(KEY_B); break;
		case 'c':  repo_dummy(KEY_C); break;
		case 'd':  repo_dummy(KEY_D); break;
		case 'e':  repo_dummy(KEY_E); break;
		case 'f':  repo_dummy(KEY_F); break;
		case 'g':  repo_dummy(KEY_G); break;
		case 'h':  repo_dummy(KEY_H); break;
		case 'i':  repo_dummy(KEY_I); break;
		case 'j':  repo_dummy(KEY_J); break;
		case 'k':  repo_dummy(KEY_K); break;
		case 'l':  repo_dummy(KEY_L); break;
		case 'm':  repo_dummy(KEY_M); break;
		case 'n':  repo_dummy(KEY_N); break;
		case 'o':  repo_dummy(KEY_O); break;
		case 'p':  repo_dummy(KEY_P); break;
		case 'q':  repo_dummy(KEY_Q); break;
		case 'r':  repo_dummy(KEY_R); break;
		case 's':  repo_dummy(KEY_S); break;
		case 't':  repo_dummy(KEY_T); break;
		case 'u':  repo_dummy(KEY_U); break;
		case 'v':  repo_dummy(KEY_V); break;
		case 'w':  repo_dummy(KEY_W); break;
		case 'x':  repo_dummy(KEY_X); break;
		case 'y':  repo_dummy(KEY_Y); break;
		case 'z':  repo_dummy(KEY_Z); break;
		case '=':  repo_dummy(KEY_EQUAL); break;
		case '-':  repo_dummy(KEY_MINUS); break;
		case '\t':  repo_dummy(KEY_TAB); break;
		case '/':  repo_dummy(KEY_SLASH); break;
		case ' ':  repo_dummy(KEY_SPACE); break;
		case '+':  repo_dummy(KEY_F1); break;
		case ',':  repo_dummy(KEY_BACKSPACE); break;
		case '.':  repo_dummy(KEY_DOT); break;
		case '%':  repo_dummy(KEY_VOLUMEUP); break;
		case '&':  repo_dummy(KEY_VOLUMEDOWN); break;
		default:
			return count;
		}
	}

	return count;
}
#endif /*CONFIG_EMXX_ANDROID*/

#define KZM9D_SW2_NAME "sw2_NMI"
static int kzm9d_sw2_irq = INT_GPIO_3;

static irqreturn_t kzm9d_sw_handler(int irq, void *dev_id)
{
	if (debug) {
		printk("%s: irq handler called\n", __FUNCTION__);
	}
#ifdef CONFIG_EMXX_ANDROID
	wake_lock_timeout(&key_lock, 15 * HZ);
#endif
	/* kick the timer right now */
	mod_timer(&kzm9d_kp_scan_timer, jiffies);
	return IRQ_HANDLED;
}

static u32 kzm9d_kp_get_data(void)
{
	u32 sw2, sw3, sw4, sw6, ret;
	u32 sw5a, sw5b, sw5c, sw5d, sw5e;

	sw2 = (gpio_get_value(GPIO_P3) == 0) ? 1 : 0;
	sw3 = (gpio_get_value(GPIO_P9) == 0) ? 1 : 0;

	gpio_set_value(GPIO_P123, 0x0);
	gpio_set_value(GPIO_P124, 0x1);
	gpio_set_value(GPIO_P125, 0x0);
	udelay(1);
	sw4 = gpio_get_value(GPIO_P126);


	gpio_set_value(GPIO_P123, 0x0);
	gpio_set_value(GPIO_P124, 0x0);
	gpio_set_value(GPIO_P125, 0x1);

	udelay(1);
	sw6 = gpio_get_value(GPIO_P126);

	gpio_set_value(GPIO_P123, 0x1);
	gpio_set_value(GPIO_P124, 0x0);
	gpio_set_value(GPIO_P125, 0x0);
	udelay(1);

	sw5a = gpio_get_value(GPIO_P126);
	sw5b = gpio_get_value(GPIO_P127);
	sw5c = gpio_get_value(GPIO_P128);
	sw5d = gpio_get_value(GPIO_P129);
	sw5e = gpio_get_value(GPIO_P130);

	ret = sw5a | (sw5b << 1) | (sw5c << 2) | (sw5d << 3) | (sw5e << 4);
	ret |= (sw6 << 5) | (sw4 << 6);
	ret |= (sw2 << 7) | (sw3 << 8);
#if 0
	if (ret != 0) {
		printk("kzm9d_keybad: %08x\n", ret);
	}
#endif
	return ret;
}

static void kzm9d_kp_report(u32 report, u32 status)
{
	int i;

	if (!status)
		return;

	for (i = 0; i<ARRAY_SIZE(kzm9d_kp_keycode); i++) {
		if (status & (1 << i)) {
			if (debug) {
				printk("%s: code=%d %s\n", report ? "Push":"Rel", kzm9d_kp_keycode[i], code_name[i]);
			}
			input_report_key(inpdev, kzm9d_kp_keycode[i], report);
		}
	}
	input_sync(inpdev);
}


static void kzm9d_kp_timer(unsigned long data)
{
	u32 push;
	u32 tmp, tmp2, tmp3;
	static u32 push_sts = 0;

	push = kzm9d_kp_get_data();

	if (push == 0) {
		if (push_sts) {
			kzm9d_kp_report(0, push_sts);
		}
		mod_timer(&kzm9d_kp_scan_timer, jiffies + ((scan_interval*HZ) / 1000));
		push_sts = 0;
	} else {
#ifdef CONFIG_EMXX_ANDROID
		wake_lock_timeout(&key_lock, 15 * HZ);
#endif
		tmp  = push_sts ^ push;
		if (tmp) {
			tmp2 = tmp & push_sts;
			tmp3 = tmp & ~push_sts;
			if (tmp2) {
				kzm9d_kp_report(0, tmp2);
				push_sts &= ~tmp2;
			}
			if (tmp3) {
				kzm9d_kp_report(1, tmp3);
				push_sts |= tmp3;
			}
		}
		mod_timer(&kzm9d_kp_scan_timer, jiffies + ((KZM9D_KP_SCAN_LOOP_TIME*HZ) / 1000));
	}
}


static int kzm9d_kp_probe(struct platform_device *dev)
{
	int i, ret;
#ifdef CONFIG_EMXX_ANDROID
	struct proc_dir_entry *key_proc;
#endif

	inpdev = input_allocate_device();
	if (!inpdev) {
		printk("%s: error allocating memory for input structure\n", __FUNCTION__);
		return -ENOMEM;
	}

	init_timer(&kzm9d_kp_scan_timer);

	inpdev->name = KZM9D_KP_NAME;
	inpdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	for (i = 0; i < ARRAY_SIZE(kzm9d_kp_keycode); i++)
		set_bit(kzm9d_kp_keycode[i], inpdev->keybit);

#ifdef CONFIG_EMXX_ANDROID
	for (i = 0; i < ARRAY_SIZE(dummy_code); i++) {
		set_bit(dummy_code[i], inpdev->keybit);
	}
#endif

	clear_bit(0, inpdev->keybit);

	ret = input_register_device(inpdev);
	if (ret != 0) {
		printk("%s: error registering with input system\n", __FUNCTION__);
		input_free_device(inpdev);
		return ret;
	}

#ifdef CONFIG_EMXX_ANDROID
	key_proc = create_proc_read_entry("key", 0, NULL, key_read_proc, NULL );
	key_proc->write_proc = key_write_proc;

	wake_lock_init(&key_lock, WAKE_LOCK_SUSPEND, "key");
#endif
	// H/W init
	gpio_direction_output(GPIO_P123, 0x0);
	gpio_direction_output(GPIO_P124, 0x0);
	gpio_direction_output(GPIO_P125, 0x0);
	gpio_direction_input(GPIO_P126);
	gpio_direction_input(GPIO_P127);
	gpio_direction_input(GPIO_P128);
	gpio_direction_input(GPIO_P129);
	gpio_direction_input(GPIO_P130);
	gpio_direction_input(GPIO_P0);
	gpio_direction_input(GPIO_P3);
	gpio_direction_input(GPIO_P9);

	mod_timer(&kzm9d_kp_scan_timer, jiffies + ((scan_interval*HZ) / 1000));


	/* request irq */
	irq_set_irq_type(kzm9d_sw2_irq, IRQ_TYPE_EDGE_FALLING);
	if (request_irq(kzm9d_sw2_irq, kzm9d_sw_handler, IRQF_DISABLED, KZM9D_SW2_NAME, NULL) < 0) {
		printk("%s: failed to request_irq\n", __FUNCTION__);
		input_free_device(inpdev);
		return -EINVAL;
	}
	return 0;
}

static int kzm9d_kp_remove(struct platform_device *pdev)
{
#ifdef CONFIG_EMXX_ANDROID
	wake_lock_destroy(&key_lock);
	remove_proc_entry("key", NULL);
#endif

	del_timer(&kzm9d_kp_scan_timer);
	input_unregister_device(inpdev);

	return 0;
}

static struct platform_driver kzm9d_kp_driver = {
	.probe 		= kzm9d_kp_probe,
	.remove 	= kzm9d_kp_remove,
	.driver = {
		.name	= KZM9D_KP_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init kzm9d_kp_init(void)
{
	return platform_driver_register(&kzm9d_kp_driver);
}

static void __exit kzm9d_kp_exit(void)
{
	platform_driver_unregister(&kzm9d_kp_driver);
}

module_init(kzm9d_kp_init);
module_exit(kzm9d_kp_exit);

module_param(scan_interval, uint, 0644);
MODULE_PARM_DESC(scan_interval, "scan interval time [ms]");

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Enable(1)/Disable(0) debug message");

MODULE_AUTHOR("KMC");
MODULE_LICENSE("GPL");
