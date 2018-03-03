/* 2013-08-20: File added and changed by Sony Corporation */
/*
 * linux/arch/arm/mach-ne1/ne1tb.c
 *
 * Copyright (C) NEC Electronics Corporation 2007, 2008
 *
 * This file is based on arch/arm/common/gic.c
 *
 * Copyright (C) 2002 ARM Limited, All Rights Reserved.
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
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/smp.h>
#include <linux/cpumask.h>

#include <asm/gpio.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <asm/hardware/gic.h>


/*
 * NaviEngine1 GPIO interrupt support
 */

#define MAX_GPIO_NR		1

#if defined(CONFIG_MACH_NE1TB)
static const unsigned int gpio_mode_ne1tb[8] = {
	(((unsigned int)(~0x1a00ffff)) | 0x1a00),			/* SCL */
	(((unsigned int)(~0xffbfffff)) | 0xffbf),			/* SCH */
	0x0200fa00,							/* PCE */
	0x00000000, 0x00000202, 0x34333333, 0x33333303,			/* IM0-IM3 */
	0x00000500							/* INH */
};
#endif

#if defined(CONFIG_MACH_NE1DB)
static const unsigned int gpio_mode_ne1db[8] = {
	(((unsigned int)(~0x1fe0ffff) | 0x1fe0),			/* SCL */
	(((unsigned int)(~0xffbfffff) | 0xffbf),			/* SCH */
	0x0200fa90,							/* PCE */
	0x00000000, 0x00000202, 0x34333333, 0x33333303,			/* IM0-IM3 */
	0x00000500							/* INH */
};
#endif

static inline unsigned int gpio_irq(unsigned int irq)
{
	return (irq - IRQ_GPIO_BASE);
}

static void gpio_ack_irq(struct irq_data *data)
{
	unsigned int irq = data->irq;
	u32 mask = 1 << gpio_irq(irq);

	writel(mask, (VA_GPIO + GPIO_INT));
}

static void gpio_mask_irq(struct irq_data *data)
{
	unsigned int irq = data->irq;
	u32 mask = 1 << gpio_irq(irq);

	writel(mask, (VA_GPIO + GPIO_IND));
}

static void gpio_unmask_irq(struct irq_data *data)
{
	unsigned int irq = data->irq;
	u32 mask = 1 << gpio_irq(irq);

	writel(mask, (VA_GPIO + GPIO_INE));
}

static int gpio_set_irq_type(struct irq_data *data, unsigned int type);

static struct irq_chip gpio_chip_ack = {
	.name			= "GIO-edge",
	.irq_ack		= gpio_ack_irq,
	.irq_mask		= gpio_mask_irq,
	.irq_unmask		= gpio_unmask_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	= NULL,
#endif
	.irq_disable		= gpio_mask_irq,
	.irq_set_type		= gpio_set_irq_type,
};

static struct irq_chip gpio_chip = {
	.name			= "GIO-level",
	.irq_ack		= gpio_mask_irq,
	.irq_mask		= gpio_mask_irq,
	.irq_mask_ack		= gpio_mask_irq,
	.irq_unmask		= gpio_unmask_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	= NULL,
#endif
	.irq_disable		= gpio_mask_irq,
	.irq_set_type		= gpio_set_irq_type,
};

static int gpio_set_irq_type(struct irq_data *data, unsigned int type)
{
	unsigned int irq = data->irq;
	unsigned int pin = gpio_irq(irq);
	unsigned int mode = 0, level_flag = 0;
	unsigned int im_addr, im_shift, im;
	unsigned int inh;
	struct irq_desc *desc;
	struct irq_chip *chip;

	printk("type GPIO=%d type=%x\n", pin, type);

	if ((irq < INT_GPIO_BASE) || (INT_GPIO_LAST < irq)) {
		return -EINVAL;
	}

	desc = irq_desc + irq;
	chip = irq_get_chip(irq);

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
		mode = 0;
		chip = &gpio_chip_ack;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		mode = 1;
		chip = &gpio_chip_ack;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		mode = 2;
		chip = &gpio_chip_ack;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		mode = 3;
		chip = &gpio_chip;
		desc->handle_irq = handle_level_irq;
		level_flag = 1;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		mode = 4;
		chip = &gpio_chip;
		desc->handle_irq = handle_level_irq;
		level_flag = 1;
		break;
	default:
		return -EINVAL;
	}

	im_addr = (unsigned int)VA_GPIO + GPIO_IM0;
	im_addr += (pin / 8) << 2;
	im_shift = (pin & 0x7) << 2;

	im = readl(im_addr);
	im &= ~(0xfU << im_shift);
	im |= (mode << im_shift);
	writel(im, im_addr);

	inh = readl(VA_GPIO + GPIO_INH);

	if (level_flag == 1) {
		inh &= ~(1U << pin);
	} else {
		inh |= (1U << pin);
	}

	writel(inh, VA_GPIO + GPIO_INH);

	return 0;
}

void __init gpio_init(unsigned int gpio_nr, void __iomem *base, unsigned int irq_offset)
{
	unsigned int max_irq, i, gpio_irq, mode, inh=0;
	const unsigned int *gpio_mode;

	if (gpio_nr >= MAX_GPIO_NR)
		BUG();

	/*
	 * initialize GPIO
	 */
	gpio_mode = gpio_mode_ne1tb;

	writel(0xffffffff, (base + GPIO_PCD));
	writel(0xffffffff, (base + GPIO_IPOLRST));
	writel(0xffffffff, (base + GPIO_OPOLRST));
	writel(gpio_mode[0], (base + GPIO_SCL));
	writel(gpio_mode[1], (base + GPIO_SCH));
	writel(gpio_mode[2], (base + GPIO_PCE));

	writel(0xffffffff, (base + GPIO_IND));
	writel(gpio_mode[3], (base + GPIO_IM0));
	writel(gpio_mode[4], (base + GPIO_IM1));
	writel(gpio_mode[5], (base + GPIO_IM2));
	writel(gpio_mode[6], (base + GPIO_IM3));
	writel(gpio_mode[7], (base + GPIO_INH));

	writel(0xffffffff, (base + GPIO_IPOLRST));
	writel(0x00000000, (base + GPIO_IPOLINV));

	writel(0xffffffff, (base + GPIO_INT));

	/*
	 * setup GPIO IRQ
	 */
	max_irq = 32;

	for (i = irq_offset, gpio_irq = 0; i < irq_offset + max_irq; i++, gpio_irq++) {
		mode = gpio_mode[3 + (gpio_irq / 8)];
		mode = (mode >> (4 * (gpio_irq & 0x7))) & 0xf;
		switch (mode) {
		case 0:
		case 1:
		case 2:
			irq_set_chip_and_handler(i, &gpio_chip_ack,
						 handle_edge_irq);
			inh |= (1U << gpio_irq);
			break;
		default:
			irq_set_chip_and_handler(i, &gpio_chip,
						 handle_level_irq);
			break;
		}
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}

	writel(inh, base + GPIO_INH);
}

static void gpio_handle_cascade_irq(unsigned int irq, struct irq_desc *desc)
{
	struct irq_chip *chip = irq_get_chip(irq);
	unsigned int cascade_irq, gpio_irq, mask;
	unsigned long status;

	/* GIC: disable (mask) and send EOI */
	chip->irq_mask(&desc->irq_data);
	chip->irq_eoi(&desc->irq_data);

	status = readl(VA_GPIO + GPIO_INT);
	status &= readl(VA_GPIO + GPIO_INE);

	for (gpio_irq = 0, mask = 1; gpio_irq < 32; gpio_irq++, mask <<= 1) {
		if (status & mask) {
			cascade_irq = gpio_irq + IRQ_GPIO_BASE;
			generic_handle_irq(cascade_irq);
		}
	}

	/* GIC: enable (unmask) */
	chip->irq_unmask(&desc->irq_data);
}

void __init gpio_cascade_irq(unsigned int gpio_nr, unsigned int irq)
{
	if (gpio_nr >= MAX_GPIO_NR)
		BUG();
	irq_set_chained_handler(irq, gpio_handle_cascade_irq);
}

