/* 2008-09-01: File added by Sony Corporation */
/*
 *  File Name       : drivers/mtd/nand/ne1-nand.c
 *  Function        : NAND Flash memory access on NE1 based devices
 *  Release Version : Ver 1.00
 *  Release Date    : 2008/06/10
 *
 *  Copyright (C) NEC Electronics Corporation 2008
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

#include <linux/device.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>

/* NE1 nand I/F register offset */
#define XROMA1	0x840
#define XROMA2	0x842
#define XROMM0	0x844
#define XROMM1	0x846
#define XROMM2	0x848
#define XROMM3	0x84A
#define XROMS	0x84C
#define XROMC	0x84E
#define XECC	0x850
#define XCS1	0x852
#define XCS2	0x854
#define XCSV	0x856
#define XFCLR	0x858
#define XTIM	0x85A
#define XTOT	0x85C
#define XSYND1	0x870
#define XSYND2	0x872
#define XECONT1	0x874
#define XECONT2	0x876
#define XVTAG1	0x880
#define XVTAG2	0x882
#define XVTAG3	0x884
#define XVTAG4	0x886
#define XERGEN1	0x890
#define XERGEN2	0x892

/* NE1 nand chip */
#define LP_OPTIONS (NAND_SAMSUNG_LP_OPTIONS | NAND_NO_READRDY | NAND_NO_AUTOINCR)
static struct nand_flash_dev ne1_nand_flash_ids =
	{"NAND 256MiB 3,3V 8-bit", 0xDA, 2048, 256, 128*1024, LP_OPTIONS};

#ifdef CONFIG_MTD_PARTITIONS
const char *part_probes[] = { "cmdlinepart", NULL, };
#endif

static struct mtd_info *ne1_nand_mtd = NULL;

static void __iomem *ne1_nand_base;
static int nand_offset = 0;

extern int nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip);
extern int nand_default_block_markbad(struct mtd_info *mtd, loff_t ofs);

static void ne1_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	u8 reg_cmd;

	if ((cmd != NAND_CMD_NONE) &&
			(cmd != NAND_CMD_SEQIN) && (cmd != NAND_CMD_ERASE1) &&
				(cmd != NAND_CMD_READ0) && (cmd != NAND_CMD_RESET)) {
		switch (cmd) {
		case NAND_CMD_READSTART: reg_cmd = 1; break;
		case NAND_CMD_PAGEPROG:  reg_cmd = 3; break;
		case NAND_CMD_ERASE2:    reg_cmd = 4; break;
		case NAND_CMD_STATUS:    reg_cmd = 5; break;
//		case NAND_CMD_RESET:     reg_cmd = 6; break;
		default:
			printk("%s: Unknown nand command. (%x)\n", __FUNCTION__, cmd);
			return;
		}
		writew(reg_cmd | 0x100, ne1_nand_base + XROMC);
	}
}

static void ne1_nand_command_lp(struct mtd_info *mtd, unsigned int command,
			    int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}

	/* Command latch cycle */
	chip->cmd_ctrl(mtd, command & 0xff, 0);

	if (column != -1 || page_addr != -1) {
		/* Serially input address */
		if (column != -1) {
			nand_offset = column;
		} else {
			nand_offset = 0;
		}
		if (page_addr != -1) {
			page_addr <<= chip->page_shift;
			writew(page_addr,  ne1_nand_base + XROMA1);
			writew(page_addr >> 16,  ne1_nand_base + XROMA2);
		}
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	switch (command) {
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_RNDIN:
	case NAND_CMD_DEPLETE1:
	case NAND_CMD_RNDOUT:
		return;

	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE2:
	case NAND_CMD_STATUS:
	case NAND_CMD_RESET:
		while (readw(ne1_nand_base + XROMC) & 0x100);
		return;

	case NAND_CMD_SEQIN:
		if (nand_offset == 0x800) {
			int i;
			for (i = 0; i<0x800; i+=2) {
				writew(0xffff, ne1_nand_base + i);
			}
		}
		return;

	case NAND_CMD_STATUS_ERROR:
	case NAND_CMD_STATUS_ERROR0:
	case NAND_CMD_STATUS_ERROR1:
	case NAND_CMD_STATUS_ERROR2:
	case NAND_CMD_STATUS_ERROR3:
		udelay(chip->chip_delay);
		return;

	case NAND_CMD_READ0:
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART, 0);
		while (readw(ne1_nand_base + XROMC) & 0x100);
		return;

	default:
		return;
	}
}

//static int nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
static int ne1_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	unsigned long timeo = jiffies;
	int status, state = chip->state;

	if (state == FL_ERASING)
		timeo += (HZ * 400) / 1000;
	else {
		timeo += (HZ * 20) / 1000;
	}
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	while (time_before(jiffies, timeo)) {
		if (readw(ne1_nand_base + XROMS) & NAND_STATUS_READY)
			break;
		cond_resched();
	}

	status = (int)(readw(ne1_nand_base + XROMS) & 0xff);
	return status;
}

//static void nand_select_chip(struct mtd_info *mtd, int chipnr)
static void ne1_nand_select_chip(struct mtd_info *mtd, int chipnr)
{
}

//static uint8_t nand_read_byte16(struct mtd_info *mtd)
static uint8_t ne1_nand_read_byte(struct mtd_info *mtd)
{
	return (uint8_t)(readw(ne1_nand_base + nand_offset++) & 0xff);
}

//static u16 nand_read_word(struct mtd_info *mtd)
static u16 ne1_nand_read_word(struct mtd_info *mtd)
{
	u16 val = readw(ne1_nand_base + nand_offset);
	nand_offset += 2;

	return val;
}

//static void nand_write_buf16(struct mtd_info *mtd, const uint8_t *buf, int len)
static void ne1_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	u16 *p = (u16 *) buf;
	volatile u16 *p2 = (u16 *)(ne1_nand_base + nand_offset);

	nand_offset += len;
	len >>= 1;

	for (i = 0; i < len; i++) {
		*p2++ = *p++;
	}
}

//static void nand_read_buf16(struct mtd_info *mtd, uint8_t *buf, int len)
static void ne1_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;
	u16 *p = (u16 *) buf;
	volatile u16 *p2 = (u16 *)(ne1_nand_base + nand_offset);

	nand_offset += len;
	len >>= 1;

	for (i = 0; i < len; i++) {
		*p++ = *p2++;
	}
}

//static int nand_verify_buf16(struct mtd_info *mtd, const uint8_t *buf, int len)
static int ne1_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	u16 *p = (u16 *) buf;
	volatile u16 *p2 = (u16 *)(ne1_nand_base + nand_offset);
	nand_offset += len;

	len >>= 1;

	for (i = 0; i < len; i++)
		if (*p++ != *p2++)
			return -EFAULT;

	return 0;
}

//static void single_erase_cmd(struct mtd_info *mtd, int page)
static void ne1_erase_cmd(struct mtd_info *mtd, int page)
{
	struct nand_chip *chip = mtd->priv;
	/* Send commands to erase a block */
	chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page);
	chip->cmdfunc(mtd, NAND_CMD_ERASE2, -1, -1);
}

static int ne1_nand_probe(struct platform_device *pdev)
{
	int err = 0;
	struct platform_nand_chip *pdata = pdev->dev.platform_data;
	struct resource *res;
	unsigned long size;
	struct nand_chip *this;
	struct mtd_partition *parts = 0;
	u32 val;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk(KERN_WARNING "%s: Unable get resource infomation.\n", __FUNCTION__);
		return -ENOMEM;
	}
	size = res->end - res->start + 1;

	/* Allocate memory for MTD device structure and private data */
	ne1_nand_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	if (!ne1_nand_mtd) {
		printk(KERN_WARNING "%s: Unable to allocate NAND MTD device structure.\n", __FUNCTION__);
		return -ENOMEM;
	}

	/* Initialize structures */
	memset((char *)ne1_nand_mtd, 0, sizeof(struct mtd_info) + sizeof(struct nand_chip));

	/* Get pointer to private data */
	this = (struct nand_chip *)(&ne1_nand_mtd[1]);

	/* Link the private data with the MTD structure */
	ne1_nand_mtd->priv = this;
	ne1_nand_mtd->owner = THIS_MODULE;

	/* io address maping */
	ne1_nand_base = ioremap_nocache(res->start, size);
	if (ne1_nand_base == 0) {
		err = -ENOMEM;
		goto out_free_info;
	}

	this->cmd_ctrl       = ne1_nand_hwcontrol;
	this->cmdfunc        = ne1_nand_command_lp;
	this->waitfunc       = ne1_nand_wait;
	this->select_chip    = ne1_nand_select_chip;
	this->read_byte      = ne1_nand_read_byte;
	this->read_word      = ne1_nand_read_word;
	this->write_buf      = ne1_nand_write_buf;
	this->read_buf       = ne1_nand_read_buf;
	this->verify_buf     = ne1_nand_verify_buf;
	this->erase_cmd      = ne1_erase_cmd;

	this->scan_bbt       = nand_default_bbt;
	this->block_bad      = nand_block_bad;
	this->block_markbad  = nand_default_block_markbad;

	this->ecc.mode       = NAND_ECC_SOFT;
	this->options        = pdata->options;
	this->chip_delay     = pdata->chip_delay;

	this->controller = &this->hwcontrol;
	spin_lock_init(&this->controller->lock);
	init_waitqueue_head(&this->controller->wq);

	//-------------------------------------------------
	// Chip ID 1st:ECh 2nd:DAh 3rd:10h 4th:95h 5th:44h
	this->chipsize  = ne1_nand_flash_ids.chipsize << 20;
	this->cellinfo  = 0x10;	// 3rd data

	ne1_nand_mtd->name      = ne1_nand_flash_ids.name;
	ne1_nand_mtd->erasesize = ne1_nand_flash_ids.erasesize;	// block size (128K)
	ne1_nand_mtd->writesize = ne1_nand_flash_ids.pagesize;
	ne1_nand_mtd->oobsize   = ne1_nand_mtd->writesize / 32;

	this->page_shift = ffs(ne1_nand_mtd->writesize) - 1;
	this->pagemask   = (this->chipsize >> this->page_shift) - 1;

	this->bbt_erase_shift = this->phys_erase_shift = ffs(ne1_nand_mtd->erasesize) - 1;
	this->chip_shift      = ffs(this->chipsize) - 1;

	this->badblockpos = ne1_nand_mtd->writesize > 512 ?
		NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

	/* Get chip options, preserve non chip based options */
	this->options &= ~NAND_CHIPOPTIONS_MSK;
	this->options |= ne1_nand_flash_ids.options & NAND_CHIPOPTIONS_MSK;
	this->options |= NAND_NO_AUTOINCR;

	this->numchips = pdata->nr_chips;
	ne1_nand_mtd->size = pdata->nr_chips * this->chipsize;

	printk(KERN_INFO "NAND device: Manufacturer ID:"
	       " 0x%02x, Chip ID: 0x%02x (%s %s)\n", NAND_MFR_SAMSUNG, ne1_nand_flash_ids.id,
	       nand_manuf_ids[1].name, ne1_nand_flash_ids.name);

	/* Initialize NAND I/F */
	val = (~(this->chipsize - 1) >> 16);
	writew(val & 0xffff, ne1_nand_base + XROMM0);
	writew(0, ne1_nand_base + XECC);

	if (nand_scan_tail(ne1_nand_mtd) != 0) {
		err = -EIO;
		goto out_iounmap;
	}

	/* Register the partitions */
#ifdef CONFIG_MTD_PARTITIONS
	err = parse_mtd_partitions(ne1_nand_mtd, part_probes, &parts, 0);
	if (err > 0) {
		add_mtd_partitions(ne1_nand_mtd, parts, err);
	} else if (pdata->nr_partitions != 0) {
		add_mtd_partitions(ne1_nand_mtd, pdata->partitions, pdata->nr_partitions);
	} else {
		add_mtd_device(ne1_nand_mtd);
	}
#else
	add_mtd_device(ne1_nand_mtd);
#endif

	return 0;

out_iounmap:
	iounmap(ne1_nand_base);
out_free_info:
	kfree(ne1_nand_mtd);

	return err;
}

static int __devexit ne1_nand_remove(struct platform_device *pdev)
{
	if (ne1_nand_mtd != 0) {
		nand_release(ne1_nand_mtd);
		iounmap(ne1_nand_base);
		kfree(ne1_nand_mtd);
		ne1_nand_mtd = NULL;
	}

	return 0;
}

static struct platform_driver ne1_nand_driver = {
	.probe		= ne1_nand_probe,
	.remove		= __devexit_p(ne1_nand_remove),
	.driver		= {
		.name	= "ne1xb-nand",
		.owner	= THIS_MODULE,
	},
};

static int __init ne1_nand_init(void)
{
	return platform_driver_register(&ne1_nand_driver);
}

static void __exit ne1_nand_exit(void)
{
	platform_driver_unregister(&ne1_nand_driver);
}

module_init(ne1_nand_init);
module_exit(ne1_nand_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("NE1 board MTD NAND driver");
