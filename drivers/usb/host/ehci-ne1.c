/*
 *  File Name		: linux/drivers/usb/host/ehci-ne1.c
 *  Function		: EHCI
 *  Release Version : Ver 1.00
 *  Release Date	: 2008/05/26
 *
 *  Copyright (C) NEC Electronics Corporation 2008
 *
 *
 *  This program is free software;you can redistribute it and/or modify it under the terms of
 *  the GNU General Public License as published by Free Softwere Foundation; either version 2
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

#include <linux/signal.h>	/* IRQF_DISABLED */
#include <linux/jiffies.h>
#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <asm/io.h>

/*-------------------------------------------------------------------------*/

static int ehci_ne1_init(struct usb_hcd *hcd)
{
	int ret;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(ehci, readl(&ehci->caps->hc_capbase));
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	ret = ehci_halt(ehci);
	if (ret)
		return ret;

	ret = ehci_init(hcd);
	if (ret)
		return ret;

	ehci_port_power(ehci, 0);

	return 0;
}

/*-------------------------------------------------------------------------*/

static int usb_hcd_ne1_probe (const struct hc_driver *driver,
			  struct platform_device *pdev)
{
	int retval, irq;
	struct usb_hcd *hcd = 0;
//	struct ehci_hcd *ehci;

	hcd = usb_create_hcd (driver, &pdev->dev, "ne1_ehci");
	if (!hcd) {
		retval = -ENOMEM;
		goto err0;
	}
	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		dev_dbg(&pdev->dev, "request_mem_region failed\n");
		retval = -EBUSY;
		goto err1;
	}

	hcd->regs = (void __iomem*)IO_ADDRESS((u32)hcd->rsrc_start);

#if 0
	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

printk("%s: param=%x %p\n", __FUNCTION__, ehci->hcs_params, ehci->regs);
#endif

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		retval = -ENXIO;
		goto err2;
	}

	retval = usb_add_hcd(hcd, irq, IRQF_DISABLED);
	if (retval)
		goto err2;

	platform_set_drvdata(pdev, hcd);

	return 0;

err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
err0:
	printk("%s: error end. (%d)\n", __FUNCTION__, retval);
	return retval;
}


static void
usb_hcd_ne1_remove (struct usb_hcd *hcd, struct platform_device *pdev)
{
	usb_remove_hcd(hcd);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
}

/*-------------------------------------------------------------------------*/
static const struct hc_driver ehci_ne1_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"NE1 EHCI",
	.hcd_priv_size =	sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ehci_irq,
	.flags =		HCD_USB2 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
//	.reset =		ehci_init,
	.reset =		ehci_ne1_init,
	.start =		ehci_run,
	.stop =			ehci_stop,
	.shutdown =		ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ehci_urb_enqueue,
	.urb_dequeue =		ehci_urb_dequeue,
	.endpoint_disable =	ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ehci_hub_status_data,
	.hub_control =		ehci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend =		ehci_bus_suspend,
	.bus_resume =		ehci_bus_resume,
#endif
};

/*-------------------------------------------------------------------------*/

static int ehci_hcd_ne1_drv_probe(struct platform_device *dev)
{
	return usb_hcd_ne1_probe(&ehci_ne1_hc_driver, dev);
}

static int ehci_hcd_ne1_drv_remove(struct platform_device *dev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);

	usb_hcd_ne1_remove(hcd, dev);
	platform_set_drvdata(dev, NULL);

	return 0;
}

/*-------------------------------------------------------------------------*/

#ifdef	CONFIG_PM

static int ehci_ne1_suspend(struct platform_device *dev, pm_message_t message)
{
	// T.B.D
	return 0;
}

static int ehci_ne1_resume(struct platform_device *dev)
{
	// T.B.D
	return 0;
}

#endif

/*-------------------------------------------------------------------------*/

static struct platform_driver ehci_hcd_ne1_driver = {
	.probe		= ehci_hcd_ne1_drv_probe,
	.remove		= ehci_hcd_ne1_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
#ifdef	CONFIG_PM
	.suspend	= ehci_ne1_suspend,
	.resume		= ehci_ne1_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ne1xb_ehci",
	},
};

