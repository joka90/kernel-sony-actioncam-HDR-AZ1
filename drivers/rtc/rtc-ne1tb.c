/* 2009-04-07: File added and changed by Sony Corporation */
/*
 *  File Name       : drivers/char/ne1tb_rtc.c
 *  Function        : NE1 Real Time Clock interface
 *  Release Version : Ver 1.00
 *  Release Date    : 2008/05/29
 *
 *  Copyright (C) NEC Electronics Corporation 2007
 *
 *
 *  This program is free software;you can redistribute it and/or modify itunder the terms of
 *  the GNU General Public License as published by Free Softwere Foundation;either version 2
 *  of License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warrnty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with this program;
 *  If not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA.
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/io.h>

#include <asm/uaccess.h>
#include <mach/hardware.h>

static DEFINE_RAW_SPINLOCK(ne1_rtc_lock);

#define RTC_BASE	IO_ADDRESS(NE1TB_BASE_RTC)
#define RTC_CLKDIV	(RTC_BASE + 0x00)
#define RTC_ADR		(RTC_BASE + 0x08)
#define RTC_CTRL	(RTC_BASE + 0x10)
#define RTC_STS		(RTC_BASE + 0x18)
#define RTC_RB		(RTC_BASE + 0x20)
#define RTC_WB		(RTC_BASE + 0x28)

/* register offset address define */
#define RTC_SEC		0x00
#define RTC_MIN		0x01
#define RTC_HOUR	0x02
#define RTC_WEEK	0x03
#define RTC_DAY		0x04
#define RTC_MONTH	0x05
#define RTC_YEAR	0x06
#define RTC_TRIM	0x07
#define RTC_ALM_WM	0x08
#define RTC_ALM_WH	0x09
#define RTC_ALM_WW	0x0A
#define RTC_ALM_DM	0x0B
#define RTC_ALM_DH	0x0C
#define RTC_RTC1	0x0E
#define RTC_RTC2	0x0F

/* RTC1 register bit define */
#define RTC1_WALE		0x80
#define RTC1_DALE		0x40
#define RTC1_1224		0x20
#define RTC1_CLEN2		0x10
#define RTC1_TEST		0x08
#define RTC1_CT_SEC		0x04
#define RTC1_CT_MIN		0x05
#define RTC1_CT_HOUR	0x06
#define RTC1_CT_MONTH	0x07
#define RTC1_CT_MASK	0x07

/* RTC2 register bit define */
#define RTC2_VDSL		0x80
#define RTC2_VDET		0x40
#define RTC2_SCRATCH	0x20
#define RTC2_XSTP		0x10
#define RTC1_CLEN1		0x08
#define RTC2_CTFG		0x04
#define RTC2_WAFG		0x02
#define RTC2_DAFG		0x01

static unsigned long epoch = 1900;	/* year corresponding to 0x00 */

static const unsigned char days_in_mo[] =
    { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static int rtc_irq = 0;
static int p_freq = 1;
static unsigned int aie_irqen;
static unsigned int pie_irqen;

#define RTC_READ(addr, val) rtc_read((addr), (unsigned char *)(&(val)));
#define RTC_WRITE(val, addr) rtc_write((addr), (unsigned char)(val));

static void rtc_read(unsigned char offset, unsigned char *val)
{
	writew(0x10, RTC_STS);
	writew(offset, RTC_ADR);
	writew(0x01, RTC_CTRL);
	while (readw(RTC_STS) & 0x10);
	*val = readw(RTC_RB);
}

static void rtc_write(unsigned char offset, unsigned char val)
{
	writew(0x10, RTC_STS);
	writew(offset, RTC_ADR);
	writew(val, RTC_WB);
	writew(0x11, RTC_CTRL);
	while (readw(RTC_STS) & 0x10);
}

static int ne1_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	raw_spin_lock_irq(&ne1_rtc_lock);
	RTC_READ(RTC_SEC,   rtc_tm->tm_sec);
	RTC_READ(RTC_MIN,   rtc_tm->tm_min);
	RTC_READ(RTC_HOUR,  rtc_tm->tm_hour);
	RTC_READ(RTC_WEEK,  rtc_tm->tm_wday);
	RTC_READ(RTC_DAY,   rtc_tm->tm_mday);
	RTC_READ(RTC_MONTH, rtc_tm->tm_mon);
	RTC_READ(RTC_YEAR,  rtc_tm->tm_year);
	raw_spin_unlock_irq(&ne1_rtc_lock);

	rtc_tm->tm_mon &= 0x1f;	/* Mask of D7(19/20) */

	rtc_tm->tm_sec  = bcd2bin(rtc_tm->tm_sec);
	rtc_tm->tm_min  = bcd2bin(rtc_tm->tm_min);
	rtc_tm->tm_hour = bcd2bin(rtc_tm->tm_hour);
	rtc_tm->tm_mday = bcd2bin(rtc_tm->tm_mday);
	rtc_tm->tm_mon  = bcd2bin(rtc_tm->tm_mon);
	rtc_tm->tm_year = bcd2bin(rtc_tm->tm_year);

	if ((rtc_tm->tm_year += (epoch - 1900)) <= 69) {
		rtc_tm->tm_year += 100;
	}
	rtc_tm->tm_mon--;

	return 0;
}

static int ne1_rtc_settime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned char mon, day, hrs, min, sec, leap_yr, wday;
	unsigned int yrs;

	yrs = rtc_tm->tm_year + 1900;
	mon = rtc_tm->tm_mon + 1;	/* tm_mon starts at zero */
	day = rtc_tm->tm_mday;
	hrs = rtc_tm->tm_hour;
	min = rtc_tm->tm_min;
	sec = rtc_tm->tm_sec;

	if (yrs < 1970) {
		return -EINVAL;
	}

	/* setting day of week */
	if (mon < 3) {
		wday = ((yrs-1) + (yrs-1)/4 - (yrs-1)/100 + (yrs-1)/400 + (13*(mon+12)+8)/5 + day) % 7;
	} else {
		wday = (yrs + yrs/4 - yrs/100 + yrs/400 + (13*mon+8)/5 + day) % 7;
	}

	leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

	if (wday >= 7) {
		return -EINVAL;
	}
	if ((mon > 12) || (day == 0)) {
		return -EINVAL;
	}
	if (day > (days_in_mo[mon] + ((mon == 2) && leap_yr))) {
		return -EINVAL;
	}
	if ((hrs >= 24) || (min >= 60) || (sec >= 60)) {
		return -EINVAL;
	}
	if ((yrs -= epoch) > 255) {
		return -EINVAL;
	}
	if (yrs > 169) {
		return -EINVAL;
	}
	if (yrs >= 100) {
		yrs -= 100;
	}

	sec = bin2bcd(sec);
	min = bin2bcd(min);
	hrs = bin2bcd(hrs);
	day = bin2bcd(day);
	mon = bin2bcd(mon);
	yrs = bin2bcd(yrs);

	raw_spin_lock_irq(&ne1_rtc_lock);
	RTC_WRITE(yrs,  RTC_YEAR);
	RTC_WRITE(mon,  RTC_MONTH);
	RTC_WRITE(day,  RTC_DAY);
	RTC_WRITE(wday, RTC_WEEK);
	RTC_WRITE(hrs,  RTC_HOUR);
	RTC_WRITE(min,  RTC_MIN);
	RTC_WRITE(sec,  RTC_SEC);
	raw_spin_unlock_irq(&ne1_rtc_lock);

	return 0;
}

static inline void ne1_rtc_setaie(unsigned int enable)
{
	unsigned char rtc1_val, rtc2_val;

	raw_spin_lock_irq(&ne1_rtc_lock);
	RTC_READ(RTC_RTC1, rtc1_val);
	RTC_READ(RTC_RTC2, rtc2_val);
	rtc1_val &= ~RTC1_WALE;
	rtc2_val &= ~RTC2_WAFG;
	if (enable) {
		rtc1_val |= RTC1_WALE;
	}
	RTC_WRITE(rtc2_val, RTC_RTC2);
	RTC_WRITE(rtc1_val, RTC_RTC1);

	if (enable && (aie_irqen == 0) && (pie_irqen == 0)) {
		enable_irq(rtc_irq);
	}

	if ((enable == 0) && (aie_irqen == 1) && (pie_irqen == 0)) {
		disable_irq(rtc_irq);
	}

	aie_irqen = enable;
	raw_spin_unlock_irq(&ne1_rtc_lock);
}

static inline void ne1_rtc_setpie(unsigned int enable)
{
	unsigned char rtc1_val, rtc2_val;

	raw_spin_lock_irq(&ne1_rtc_lock);
	RTC_READ(RTC_RTC1, rtc1_val);
	RTC_READ(RTC_RTC2, rtc2_val);
	rtc1_val &= ~RTC1_CT_MASK;
	if (enable) {
		switch (p_freq) {
		case 1:
			rtc1_val |= RTC1_CT_SEC;
			break;
		case 60:
			rtc1_val |= RTC1_CT_MIN;
			break;
		case 60*60:
			rtc1_val |= RTC1_CT_HOUR;
			break;
		case 60*60*24*30:
			rtc1_val |= RTC1_CT_MONTH;
			break;
		}
	}

	rtc2_val &= ~RTC2_CTFG;
	RTC_WRITE(rtc2_val, RTC_RTC2);
	RTC_WRITE(rtc1_val, RTC_RTC1);
	if (enable && (aie_irqen == 0) && (pie_irqen == 0)) {
		enable_irq(rtc_irq);
	}

	if ((enable == 0) && (aie_irqen == 0) && (pie_irqen == 1)) {
		disable_irq(rtc_irq);
	}

	pie_irqen = enable;
	raw_spin_unlock_irq(&ne1_rtc_lock);

}

static inline int ne1_rtc_setfreq(unsigned int freq)
{
	switch (freq) {
	case 1:
	case 60:
	case 60*60:
	case 60*60*30:
		break;
	default:
		return -EINVAL;
	}

	p_freq = freq;

	return 0;
}

static int ne1_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *alm_tm = &alrm->time;

	raw_spin_lock_irq(&ne1_rtc_lock);
	RTC_READ(RTC_ALM_WM, alm_tm->tm_min);
	RTC_READ(RTC_ALM_WH, alm_tm->tm_hour);
	RTC_READ(RTC_ALM_WW, alm_tm->tm_wday);
	raw_spin_unlock_irq(&ne1_rtc_lock);

	alm_tm->tm_min  = bcd2bin(alm_tm->tm_min);
	alm_tm->tm_hour = bcd2bin(alm_tm->tm_hour);

	alm_tm->tm_wday &= 0x7F;
	if(alm_tm->tm_wday) {
		if(alm_tm->tm_wday == 0x7F)
			alm_tm->tm_wday = -1;
		else
			alm_tm->tm_wday = ilog2(alm_tm->tm_wday);
	}

	return 0;
}

static int ne1_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *alm_tm = &alrm->time;
	unsigned char hrs, min, wday;

	hrs = alm_tm->tm_hour;
	min = alm_tm->tm_min;
	if (alm_tm->tm_wday == -1) {
		wday = 0x7f;	/* All */
	} else {
		if (alm_tm->tm_wday > 6)
			return -EINVAL;
		wday = 1 << alm_tm->tm_wday;
	}

	if (hrs >= 24)
		return -EINVAL;
	if (min >= 60)
		return -EINVAL;

	min = bin2bcd(min);
	hrs = bin2bcd(hrs);

	raw_spin_lock_irq(&ne1_rtc_lock);
	RTC_WRITE(wday, RTC_ALM_WW);
	RTC_WRITE(hrs,  RTC_ALM_WH);
	RTC_WRITE(min,  RTC_ALM_WM);
	raw_spin_unlock_irq(&ne1_rtc_lock);

	if (alrm->enabled) {
		ne1_rtc_setaie(1);
	} else {
		ne1_rtc_setaie(0);
	}

	return 0;
}

static irqreturn_t ne1_rtc_interrupt(int irq, void *dev_id)
{
	unsigned long events = RTC_IRQF;
	unsigned char rtc2_val = 0;
	struct rtc_device *rtc = dev_id;

	raw_spin_lock(&ne1_rtc_lock);
	RTC_READ(RTC_RTC2, rtc2_val);

	if (!(rtc2_val & (RTC2_CTFG | RTC2_WAFG)))
		return IRQ_NONE;

	if (rtc2_val & RTC2_CTFG) {
		rtc2_val &= ~RTC2_CTFG;
		events |= RTC_PF;
	}
	if (rtc2_val & RTC2_WAFG) {
		rtc2_val &= ~RTC2_WAFG;
		events |= RTC_AF;
	}

	RTC_WRITE(rtc2_val, RTC_RTC2);
	raw_spin_unlock(&ne1_rtc_lock);

	rtc_update_irq(rtc, 1, events);

	return IRQ_HANDLED;
}


static int ne1_rtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case RTC_AIE_ON:
	case RTC_AIE_OFF:
		ne1_rtc_setaie((cmd == RTC_AIE_ON) ? 1 : 0);
		break;

	case RTC_PIE_ON:
	case RTC_PIE_OFF:
		ne1_rtc_setpie((cmd == RTC_PIE_ON) ? 1 : 0);
		break;

	case RTC_IRQP_READ:
		return  put_user(p_freq, (unsigned long __user *)arg);

	case RTC_IRQP_SET:
		if (!arg) {
			return -EINVAL;
		}
		return ne1_rtc_setfreq(arg);
		break;
	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}

static int ne1_rtc_proc(struct device *dev, struct seq_file *seq)
{

	seq_printf(seq, "alarm_IRQ\t: %s\n", aie_irqen ? "yes" : "no" );
	seq_printf(seq, "periodic_IRQ\t: %s\n", pie_irqen ? "yes" : "no" );
	seq_printf(seq, "periodic_freq\t: %s\n",
		   (p_freq == 1) ? "1sec" : ((p_freq == 60) ? "1min" :
   	           ((p_freq == 60*60) ? "1hour" : ((p_freq == 60*60*24*30) ? "1month" : "not set"))));

	return 0;
}

static int ne1_rtc_open(struct device *dev)
{
	int ret;

	ret = request_irq(rtc_irq, ne1_rtc_interrupt, IRQF_DISABLED, "rtc", dev);
	if (ret < 0) {
		printk(KERN_ERR "rtc: request_irq error! (%d)\n", rtc_irq);
		return ret;
	}
	disable_irq(rtc_irq);

	return 0;
}

static void ne1_rtc_release(struct device *dev)
{
	ne1_rtc_setaie(0);
	ne1_rtc_setpie(0);

	free_irq(rtc_irq, dev);
}

static struct rtc_class_ops ne1tb_rtc_ops = {
	.open       = ne1_rtc_open,
	.release    = ne1_rtc_release,
	.ioctl      = ne1_rtc_ioctl,
	.read_time  = ne1_rtc_gettime,
	.set_time   = ne1_rtc_settime,
	.read_alarm = ne1_rtc_getalarm,
	.set_alarm  = ne1_rtc_setalarm,
	.proc       = ne1_rtc_proc,
};

static int ne1_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device	*rtc;

	rtc_irq = platform_get_irq(pdev, 0);
	if (rtc_irq < 0) {
		return -ENODEV;
	}

	rtc = rtc_device_register(pdev->name, &pdev->dev,
			&ne1tb_rtc_ops, THIS_MODULE);

	if (IS_ERR(rtc)) {
		pr_debug("%s: can't register RTC device, err %ld\n",
			pdev->name, PTR_ERR(rtc));
		goto fail;
	}

	platform_set_drvdata(pdev, rtc);

	aie_irqen = 0;
	pie_irqen = 0;

	RTC_WRITE(0x00, RTC_TRIM);
	RTC_WRITE(RTC1_1224, RTC_RTC1);
	RTC_WRITE(0, RTC_RTC2);

fail:
	return 0;
}

static int ne1_rtc_remove(struct platform_device *pdev)
{
	struct rtc_device	*rtc = platform_get_drvdata(pdev);;

	rtc_device_unregister(rtc);

	return 0;
}

static struct platform_driver ne1_rtcdrv = {
	.probe		= ne1_rtc_probe,
	.remove		= ne1_rtc_remove,
	.driver		= {
		.name	= "ne1xb_rtc",
		.owner	= THIS_MODULE,
	},
};

static int __init ne1_rtc_init(void)
{
	return platform_driver_register(&ne1_rtcdrv);
}

static void __exit ne1_rtc_exit(void)
{
	platform_driver_unregister(&ne1_rtcdrv);
}

module_init(ne1_rtc_init);
module_exit(ne1_rtc_exit);

MODULE_DESCRIPTION("NE1 board RTC Driver");
MODULE_LICENSE("GPL");
