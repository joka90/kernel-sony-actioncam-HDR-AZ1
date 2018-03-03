/* 2008-09-01: File added by Sony Corporation */
/*
 *  File Name       : linux/drivers/mtd/maps/ne1-flash.c
 *  Function        : Flash memory access on ne1 based devices
 *  Release Version : Ver 1.00
 *  Release Date    : 2006/02/22
 *
 *  Copyright (C) NEC Electronics Corporation 2006
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

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/slab.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <mach/hardware.h>
#include <asm/mach/flash.h>

#ifdef CONFIG_MTD_PARTITIONS
static const char *part_probes[] = { /* "RedBoot", */ "cmdlinepart", NULL };
#endif

struct ne1_flash_info {
	struct mtd_partition *parts;
	struct mtd_info *mtd;
	struct map_info map;
};

static void ne1_set_vpp(struct map_info *map, int enable)
{
}

static int ne1_flash_probe(struct platform_device *pdev)
{
	int err;
	struct ne1_flash_info *info;
	struct flash_platform_data *pdata = pdev->dev.platform_data;
	struct resource *res = pdev->resource;
	unsigned long size = res->end - res->start + 1;

	info = kmalloc(sizeof(struct ne1_flash_info), GFP_KERNEL);
	if (!info) {
		return -ENOMEM;
	}

	memset(info, 0, sizeof(struct ne1_flash_info));

	if (!request_mem_region(res->start, size, "flash")) {
		err = -EBUSY;
		goto out_free_info;
	}

	info->map.virt = ioremap(res->start, size);
	if (!info->map.virt) {
		err = -ENOMEM;
		goto out_release_mem_region;
	}
	info->map.name      = pdev->dev.bus_id;
	info->map.phys      = res->start;
	info->map.size      = size;
	info->map.bankwidth = pdata->width;
	info->map.set_vpp	= ne1_set_vpp;

	simple_map_init(&info->map);
	info->mtd = do_map_probe(pdata->map_name, &info->map);
	if (!info->mtd) {
		err = -EIO;
		goto out_iounmap;
	}
	info->mtd->owner = THIS_MODULE;

#ifdef CONFIG_MTD_PARTITIONS
	err = parse_mtd_partitions(info->mtd, part_probes, &info->parts, 0);
	if (err > 0) {
		add_mtd_partitions(info->mtd, info->parts, err);
	} else if (pdata->parts != 0) {
		add_mtd_partitions(info->mtd, pdata->parts, pdata->nr_parts);
	} else
#endif
		add_mtd_device(info->mtd);

	platform_set_drvdata(pdev, info);

	return 0;

out_iounmap:
	iounmap(info->map.virt);
out_release_mem_region:
	release_mem_region(res->start, size);
out_free_info:
	kfree(info);

	return err;
}

static int __devexit ne1_flash_remove(struct platform_device *pdev)
{
	struct ne1_flash_info *info = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	if (info) {
		if (info->parts) {
			del_mtd_partitions(info->mtd);
			kfree(info->parts);
		} else
			del_mtd_device(info->mtd);
		map_destroy(info->mtd);
		release_mem_region(info->map.phys, info->map.size);
		iounmap((void __iomem *) info->map.virt);
		kfree(info);
	}

	return 0;
}

static struct platform_driver ne1_flash_driver = {
	.probe  = ne1_flash_probe,
	.remove = __devexit_p(ne1_flash_remove),
	.driver = {
		.name   = "ne1xb-flash",
	},
};

static int __init ne1_flash_init(void)
{
	return platform_driver_register(&ne1_flash_driver);
}

static void __exit ne1_flash_exit(void)
{
	platform_driver_unregister(&ne1_flash_driver);
}

module_init(ne1_flash_init);
module_exit(ne1_flash_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTD NOR map driver for NE1 series boards");

