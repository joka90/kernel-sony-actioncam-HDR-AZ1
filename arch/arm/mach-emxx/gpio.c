/* 2011-08-31: File added and changed by Sony Corporation */
/*
 *  File Name       : arch/arm/mach-emxx/gpio.c
 *  Function        : gpio
 *  Release Version : Ver 1.01
 *  Release Date    : 2010/02/05
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

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <asm/mach/irq.h>

/*#define GPIO_DEBUG 1*/
#ifdef GPIO_DEBUG
#define DPRINT(FMT, ARGS...) printk(KERN_INFO "%s(): " FMT, __func__, ##ARGS)
#else
#define DPRINT(FMT, ARGS...)
#endif

#define PIN_MASK(X)	(1U << ((X) & 0x1f))	/* IIA/IEN/IDS/PLS/... */


static DEFINE_RAW_SPINLOCK(emxx_gio_lock);

static void emxx_gio_irq_ack(struct irq_data *d);
static void emxx_gio_irq_mask(struct irq_data *d);
static void emxx_gio_irq_unmask(struct irq_data *d);
static int emxx_gio_set_irq_type(struct irq_data *d, unsigned int type);

static struct irq_chip emxx_gio_chip_ack = {
	.name     = "GIO-edge",
	.irq_ack      = emxx_gio_irq_ack,
	.irq_mask     = emxx_gio_irq_mask,
	.irq_unmask   = emxx_gio_irq_unmask,
	.irq_set_type = emxx_gio_set_irq_type,
	.irq_disable  = emxx_gio_irq_mask,
};

static struct irq_chip emxx_gio_chip = {
	.name     = "GIO-level",
	.irq_ack      = emxx_gio_irq_mask,
	.irq_mask     = emxx_gio_irq_mask,
	.irq_unmask   = emxx_gio_irq_unmask,
	.irq_set_type = emxx_gio_set_irq_type,
	.irq_disable  = emxx_gio_irq_mask,
};

static uint32_t accept_bits[5] = { 0, 0, 0, 0, 0};


/* called from set_irq_type() */
static int emxx_gio_set_irq_type(struct irq_data *data, unsigned int type)
{
	unsigned int irq = data->irq;
	unsigned int pin = irq - INT_GPIO_BASE;
	unsigned int mode = 0;
	unsigned long flag;
	unsigned int oiia;
	unsigned int x;
	unsigned int mask;
	unsigned int idtshift;
	unsigned int idt, iia, iir;
	struct irq_desc *desc;

	DPRINT("Enter\n");

	if ((irq < INT_GPIO_BASE) || (INT_GPIO_LAST < irq))
		return -EINVAL;

	desc = irq_to_desc(irq);

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
		mode |= 0x00;
		data->chip = &emxx_gio_chip_ack;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		mode |= 0x01;
		data->chip = &emxx_gio_chip_ack;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		mode |= 0x02;
		data->chip = &emxx_gio_chip;
		desc->handle_irq = handle_level_irq;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		mode |= 0x03;
		data->chip = &emxx_gio_chip;
		desc->handle_irq = handle_level_irq;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		mode |= 0x04;
		data->chip = &emxx_gio_chip_ack;
		desc->handle_irq = handle_edge_irq;
		break;
	default:
		return -EINVAL;
	}

	mask = PIN_MASK(pin);
	idtshift = (pin & 0x7) << 2;	/* IDT shift bit */

	raw_spin_lock_irqsave(&emxx_gio_lock, flag);

	idt = VA_GIO + (GIO_OFFSET * PIN_INDEX(pin)) +
			((pin >> 1) & 0xc) + GIO_IDT0;
	iia = VA_GIO + (GIO_OFFSET * PIN_INDEX(pin)) + GIO_IIA;
	iir = VA_GIO + (GIO_OFFSET * PIN_INDEX(pin)) + GIO_IIR;

	/* IIA enable -> disable */
	oiia = __raw_readl(iia);
	if ((oiia & mask) != 0)
		__raw_writel((oiia & ~(mask)), iia);

	/* set IDT */
	x = __raw_readl(idt);
	if ((x & (0xfU << idtshift)) != (mode << idtshift)) {
		x &= ~(0xfU << idtshift);
		x |= (mode << idtshift);
		__raw_writel(x, idt);
	}
	DPRINT("PIN %d Set IDT, addr=0x%08x, data=0x%08x\n",
					pin, idt, __raw_readl(idt));

	/* Interrupt clear */
	__raw_writel(mask, iir);

	/* Restore if changed */
	if ((oiia & mask) != 0)
		__raw_writel(oiia, iia);

	raw_spin_unlock_irqrestore(&emxx_gio_lock, flag);

	DPRINT("Exit\n");

	return 0;
}

static void emxx_gio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	uint32_t ist, gio_irq, offset;
	struct irq_chip *chip = irq_desc_get_chip(desc);

	DPRINT("Enter\n");

	if ((irq != INT_GIO0) && (irq != INT_GIO1) &&
		 (irq != INT_GIO2) && (irq != INT_GIO3) &&
		 (irq != INT_GIO4) && (irq != INT_GIO5) &&
		 (irq != INT_GIO6) && (irq != INT_GIO7) &&
		 (irq != INT_GIO8) && (irq != INT_GIO9)) {
		return;
	}
	chained_irq_enter(chip, desc);
	offset = VA_GIO + GIO_MST;

	do {
		switch (irq) {
		case INT_GIO0:
			gio_irq = INT_GPIO_0;
			ist = __raw_readl(GIO_000_OFFSET + offset) & 0xffffU;
			break;
		case INT_GIO1:
			gio_irq = INT_GPIO_16;
			ist = __raw_readl(GIO_000_OFFSET + offset) >> 16;
			break;
		case INT_GIO2:
			gio_irq = INT_GPIO_32;
			ist = __raw_readl(GIO_032_OFFSET + offset) & 0xffffU;
			break;
		case INT_GIO3:
			gio_irq = INT_GPIO_48;
			ist = __raw_readl(GIO_032_OFFSET + offset) >> 16;
			break;
		case INT_GIO4:
			gio_irq = INT_GPIO_64;
			ist = __raw_readl(GIO_064_OFFSET + offset) & 0xffffU;
			break;
		case INT_GIO5:
			gio_irq = INT_GPIO_80;
			ist = __raw_readl(GIO_064_OFFSET + offset) >> 16;
			break;
		case INT_GIO6:
			gio_irq = INT_GPIO_96;
			ist = __raw_readl(GIO_096_OFFSET + offset) & 0xffffU;
			break;
		case INT_GIO7:
			gio_irq = INT_GPIO_112;
			ist = __raw_readl(GIO_096_OFFSET + offset) >> 16;
			break;
		case INT_GIO8:
			gio_irq = INT_GPIO_128;
			ist = __raw_readl(GIO_128_OFFSET + offset) & 0xffffU;
			break;
		case INT_GIO9:
			gio_irq = INT_GPIO_144;
			ist = __raw_readl(GIO_128_OFFSET + offset) >> 16;
			break;
		}

		if (ist == 0)
			break;

		while (ist) {
			if ((ist & 1) != 0) {
				DPRINT("PIN %d interrupt handler\n",
						gio_irq - INT_GPIO_0);
				generic_handle_irq(gio_irq);
			}
			ist >>= 1;
			gio_irq++;
		}
	} while (1);

	chained_irq_exit(chip, desc);

	DPRINT("Exit\n");
}

static void emxx_gio_irq_ack(struct irq_data *d)
{
	unsigned int irq = d->irq;

	unsigned int pin = irq - INT_GPIO_BASE;
	unsigned int mask = PIN_MASK(pin);
	unsigned int offset;

	DPRINT("Enter\n");

	if ((irq < INT_GPIO_BASE) || (INT_GPIO_LAST < irq))
		return;

	offset = (GIO_OFFSET * PIN_INDEX(pin)) + GIO_IIR;
	__raw_writel(mask, VA_GIO + offset);

	DPRINT("Exit\n");
}

static void emxx_gio_irq_mask(struct irq_data *d)
{
	unsigned int irq = d->irq;
	unsigned int pin = irq - INT_GPIO_BASE;
	unsigned int mask = PIN_MASK(pin);
	unsigned int offset;

	DPRINT("Enter\n");

	if ((irq < INT_GPIO_BASE) || (INT_GPIO_LAST < irq))
		return;

	/* disable */
	offset = (GIO_OFFSET * PIN_INDEX(pin)) + GIO_IDS;
	__raw_writel(mask, VA_GIO + offset);

	DPRINT("Exit\n");
}

static void emxx_gio_irq_unmask(struct irq_data *d)
{
	unsigned int irq = d->irq;
	unsigned int iia;
	unsigned int pin = irq - INT_GPIO_BASE;
	unsigned int mask = PIN_MASK(pin);
	unsigned long flag;
	unsigned int iia_addr, ien_addr;
	uint32_t *accept;

	DPRINT("Enter\n");

	if ((irq < INT_GPIO_BASE) || (INT_GPIO_LAST < irq))
		return;

	/* IRQ enable */
	iia_addr = VA_GIO + (GIO_OFFSET * PIN_INDEX(pin)) + GIO_IIA;
	ien_addr = VA_GIO + (GIO_OFFSET * PIN_INDEX(pin)) + GIO_IEN;
	accept = (accept_bits + PIN_INDEX(pin));

	raw_spin_lock_irqsave(&emxx_gio_lock, flag);
	if ((*accept & mask) == 0) {
		iia = __raw_readl(iia_addr);
		__raw_writel((iia | mask), iia_addr);
		*accept |= mask;
	}
	raw_spin_unlock_irqrestore(&emxx_gio_lock, flag);

	__raw_writel(mask, ien_addr);

	DPRINT("PIN %d irq unmask, addr=0x%08x, mask=0x%08x\n",
				pin, ien_addr, mask);

	DPRINT("Exit\n");
}

static int __init emxx_gio_init(void)
{
	unsigned int i, gpio_num = 0, level_irq, idt_val[20], idt;
	unsigned int offset;

	DPRINT("Enter\n");

	for (i = 0; i < 20; i += 4) {
		offset = (i / 4) * GIO_OFFSET;
		idt_val[i + 0] = __raw_readl(VA_GIO + offset + GIO_IDT0);
		idt_val[i + 1] = __raw_readl(VA_GIO + offset + GIO_IDT1);
		idt_val[i + 2] = __raw_readl(VA_GIO + offset + GIO_IDT2);
		idt_val[i + 3] = __raw_readl(VA_GIO + offset + GIO_IDT3);
	}

	/* setup default GIO Interrupt modes */
	for (i = INT_GPIO_BASE; i <= INT_GPIO_LAST; i++, gpio_num++) {
		level_irq = 0;
		idt = idt_val[gpio_num / 8];
		if (idt & (2 << (4 * (gpio_num & 0x7))))
			level_irq = 1;

		if (level_irq) {
			irq_set_chip(i, &emxx_gio_chip_ack);
			irq_set_chip(i, &emxx_gio_chip);
			irq_set_handler(i, handle_level_irq);
		} else {
			irq_set_chip(i, &emxx_gio_chip);
			irq_set_chip(i, &emxx_gio_chip_ack);
			irq_set_handler(i, handle_edge_irq);
		}
		/* AA need to check*/
		set_irq_flags(i, IRQF_VALID);
	}

	irq_set_chained_handler(INT_GIO0, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO1, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO2, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO3, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO4, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO5, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO6, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO7, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO8, emxx_gio_irq_handler);
	irq_set_chained_handler(INT_GIO9, emxx_gio_irq_handler);

	DPRINT("Exit\n");

	return 0;
}

arch_initcall(emxx_gio_init);

#ifdef CONFIG_SNSC_SSBOOT

#define MAX_GIO_PORT		5

static struct emxx_gio_ctx {
	u_int32_t iim[MAX_GIO_PORT];
	u_int32_t iia[MAX_GIO_PORT];
	u_int32_t idt[MAX_GIO_PORT][4];
} gio_ctx;

int
emxx_gio_suspend(void)
{
	struct emxx_gio_ctx *gio;
	unsigned long base;
	int i;

	gio = &gio_ctx;

	for (i = 0; i < MAX_GIO_PORT; i ++) {

		base = VA_GIO + i * GIO_OFFSET;

		gio->idt[i][0] = __raw_readl(base + GIO_IDT0);
		gio->idt[i][1] = __raw_readl(base + GIO_IDT1);
		gio->idt[i][2] = __raw_readl(base + GIO_IDT2);
		gio->idt[i][3] = __raw_readl(base + GIO_IDT3);

		gio->iia[i] = __raw_readl(base + GIO_IIA);
		gio->iim[i] = __raw_readl(base + GIO_IIM);
	}

	return 0;
}

int
emxx_gio_resume(void)
{
	struct emxx_gio_ctx *gio;
	unsigned long base;
	int i;

	gio = &gio_ctx;

	for (i = 0; i < MAX_GIO_PORT; i ++) {

		base = VA_GIO + i * GIO_OFFSET;

		__raw_writel(gio->idt[i][0], base + GIO_IDT0);
		__raw_writel(gio->idt[i][1], base + GIO_IDT1);
		__raw_writel(gio->idt[i][2], base + GIO_IDT2);
		__raw_writel(gio->idt[i][3], base + GIO_IDT3);

		/* try to clear all interrupt */
		__raw_writel(__raw_readl(base + GIO_RAW), base + GIO_IIR);

		__raw_writel(gio->iia[i], base + GIO_IIA);
		__raw_writel(gio->iim[i], base + GIO_IEN); /* retore to IEN */
	}

	return 0;
}
#endif /* CONFIG_SNSC_SSBOOT */
