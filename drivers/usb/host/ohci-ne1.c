/*
 *  File Name		: linux/drivers/usb/host/ohci-ne1.c
 *  Function		: OHCI
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

static int ohci_ne1_init(struct usb_hcd *hcd)
{
	struct ohci_hcd		*ohci = hcd_to_ohci(hcd);
	u32	rh;
	int ret;

	if ((ret = ohci_init(ohci)) < 0) {
		return ret;
	}

	rh = roothub_a (ohci);

	rh &= ~RH_A_NPS;
	ohci_writel(ohci, rh, &ohci->regs->roothub.a);
	distrust_firmware = 0;

	return 0;
}

/*-------------------------------------------------------------------------*/

static int
ohci_ne1_start (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		ret;

	if ((ret = ohci_run (ohci)) < 0) {
		dev_err(hcd->self.controller, "can't start\n");
		ohci_stop (hcd);
		return ret;
	}

	return 0;
}


/*-------------------------------------------------------------------------*/

static int usb_hcd_ne1_probe (const struct hc_driver *driver,
			  struct platform_device *pdev)
{
	int retval, irq;
	struct usb_hcd *hcd = 0;

	hcd = usb_create_hcd (driver, &pdev->dev, "ne1_ohci");
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

	ohci_hcd_init(hcd_to_ohci(hcd));

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
static const struct hc_driver ohci_ne1_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"NE1 OHCI",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.reset =		ohci_ne1_init,	// É¬Í×¡©
	.start =		ohci_ne1_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};

/*-------------------------------------------------------------------------*/

static int ohci_hcd_ne1_drv_probe(struct platform_device *dev)
{
	return usb_hcd_ne1_probe(&ohci_ne1_hc_driver, dev);
}

static int ohci_hcd_ne1_drv_remove(struct platform_device *dev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);

	usb_hcd_ne1_remove(hcd, dev);
	platform_set_drvdata(dev, NULL);

	return 0;
}

/*-------------------------------------------------------------------------*/

#ifdef	CONFIG_PM
static int ohci_ne1_suspend(struct platform_device *dev, pm_message_t message)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(platform_get_drvdata(dev));

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	ohci_to_hcd(ohci)->state = HC_STATE_SUSPENDED;
	dev->dev.power.power_state = PMSG_SUSPEND;

	return 0;
}

static int ohci_ne1_resume(struct platform_device *dev)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(platform_get_drvdata(dev));

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	dev->dev.power.power_state = PMSG_ON;
	usb_hcd_resume_root_hub(platform_get_drvdata(dev));

	return 0;
}
#endif

/*-------------------------------------------------------------------------*/

static struct platform_driver ohci_hcd_ne1_driver = {
	.probe		= ohci_hcd_ne1_drv_probe,
	.remove		= ohci_hcd_ne1_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
#ifdef	CONFIG_PM
	.suspend	= ohci_ne1_suspend,
	.resume		= ohci_ne1_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ne1xb_ohci",
	},
};

