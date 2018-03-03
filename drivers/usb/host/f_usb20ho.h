/*
 * drivers/usb/host/f_usb20ho.h
 *
 * Copyright (C) 2011-2012 FUJITSU SEMICONDUCTOR LIMITED
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _F_USB20HO_H
#define _F_USB20HO_H

/*
 * [notice]:There is a configuration function at the bottom
 */

/************************************************************************************************/
/* Implementation of here to F_USB20HO driver definition                        */

/* F_USB20HO driver version */
#define F_USB20HO_CONFIG_EHCI_DRIVER_VERSION	"1.0.0" 			/**< EHCI_DRIVER_VERSION */
#define F_USB20HO_CONFIG_OHCI_DRIVER_VERSION	"1.0.0" 			/**< OHCI_DRIVER_VERSION */

#define F_USB_HO_EHCI_IRQ		230U					/**< EHCI IRQ num */
#define F_USB_HO_OHCI_IRQ		231U					/**< OHCI IRQ num */

#define F_USB_HO_PHY_RESET_TIME 	1U					/**< PHY reset ms time */

/* So far, it is implementation of F_USB20HO driver definition                    */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HO register                                   */

/* EHCI Operational Registers */
#define F_USB20HO_EHCI_BASE			0x0000					/**< EHCI Registers Base	    */
#define F_USB20HO_HCCAPBASE			(F_USB20HO_EHCI_BASE + 0x0000)		/**< Capability Registers	    */
#define F_USB20HO_HCSPARAMS			(F_USB20HO_EHCI_BASE + 0x0004U)		/**< Structural Parameters Register */
#define F_USB20HO_HCCPARAMS			(F_USB20HO_EHCI_BASE + 0x0008U)		/**< Capability Parameters Register */
#define F_USB20HO_EHCI_USBOP_BASE		(F_USB20HO_EHCI_BASE + 0x0010U)		/**< USBOPBASE			    */
#define F_USB20HO_USBCMD			(F_USB20HO_EHCI_USBOP_BASE + 0x0000)	/**< USB Command Register	    */
#define F_USB20HO_USBSTS			(F_USB20HO_EHCI_USBOP_BASE + 0x0004U)	/**< USB Status Register	    */
#define F_USB20HO_USBINTR			(F_USB20HO_EHCI_USBOP_BASE + 0x0008U)	/**< USB Interrupt Enable Register  */
#define F_USB20HO_FRINDEX			(F_USB20HO_EHCI_USBOP_BASE + 0x000cU)	/**< USB Frame Index Register	    */
#define F_USB20HO_CTRLDSSEGMENT			(F_USB20HO_EHCI_USBOP_BASE + 0x0010U)	/**< 4G Segment Selector Register   */
#define F_USB20HO_PERIODICLISTBASE		(F_USB20HO_EHCI_USBOP_BASE + 0x0014U)	/**< Periodic Frame List Base Address Register */
#define F_USB20HO_ASYNCLISTADDR			(F_USB20HO_EHCI_USBOP_BASE + 0x0018U)	/**< Asynchronous List Address Register */
#define F_USB20HO_CONFIGFLAG			(F_USB20HO_EHCI_USBOP_BASE + 0x0040U)	/**< Configured Flag Register	    */
#define F_USB20HO_PORTSC_1			(F_USB20HO_EHCI_USBOP_BASE + 0x0044U)	/**< Port Status/Control Register   */
#define F_USB20HO_PORT_TEST_MASK		(0xfU << 16U)				/**< TEST MODE MASK		    */
#define F_USB20HO_PORT_TEST_J_STATE		(1U << 16U)				/**< set TEST J 		    */
#define F_USB20HO_PORT_TEST_K_STATE		(2U << 16U)				/**< set TEST K 		    */
#define F_USB20HO_PORT_TEST_SE0_NAK		(3U << 16U)				/**< set TEST SE0 NAK		    */
#define F_USB20HO_PORT_TEST_PACKET		(4U << 16U)				/**< set TEST PACKET		    */
#define F_USB20HO_PORT_TEST_FORCE_ENABLE	(5U << 16U)				/**< set TEST FORCE ENABE	    */
#define F_USB20HO_RESERVED			(F_USB20HO_EHCI_USBOP_BASE + 0x0048U)	/**< Reserved			    */
#define F_USB20HO_INSNREG00			(F_USB20HO_EHCI_USBOP_BASE + 0x0080U)	/**< Programmable Microframe Base Value Register */
#define F_USB20HO_INSNREG01			(F_USB20HO_EHCI_USBOP_BASE + 0x0084U)	/**< Programmable Packet Buffer OUT/IN Threshold Register */
#define F_USB20HO_INSNREG02			(F_USB20HO_EHCI_USBOP_BASE + 0x0088U)	/**< Programmable Packet Buffer Depth Register */
#define F_USB20HO_INSNREG03			(F_USB20HO_EHCI_USBOP_BASE + 0x008cU)	/**< Time-Available Offset Register */
#define F_USB20HO_INSNREG04			(F_USB20HO_EHCI_USBOP_BASE + 0x0090U)	/**< Debug Register		    */
#define F_USB20HO_INSNREG05			(F_USB20HO_EHCI_USBOP_BASE + 0x0094U)	/**< UTMI Control Status Register   */

/* OHCI Operational Registers */
#define F_USB20HO_OHCI_BASE			0x1000U				/**< OHCI Registers Base */
#define F_USB20HO_HCREVISION			0x0000				/**< Revision		 */
#define F_USB20HO_HCCONTROL			0x0004U				/**< Control		 */
#define F_USB20HO_HCCommandStatus		0x0008U				/**< Command Status	 */
#define F_USB20HO_HCINTERRUPTSTATUS		0x000cU				/**< Interrup tStatus	 */
#define F_USB20HO_HCINTERRUPTENABLE		0x0010U				/**< Interrupt Enable	 */
#define F_USB20HO_HCHCINTERRUPTDISABLE		0x0014U				/**< Interrupt Disable	 */
#define F_USB20HO_HCHCCA			0x0018U				/**< HCCA		 */
#define F_USB20HO_HCPERIODCURRENTED		0x001cU				/**< Period Current ED	 */
#define F_USB20HO_HCCONTROLHEADED		0x0020U				/**< Control Head ED	 */
#define F_USB20HO_HCCONTROLCURRENTED		0x0024U				/**< Control Current ED  */
#define F_USB20HO_HCBULKHEADED			0x0028U				/**< Bulk Head ED	 */
#define F_USB20HO_HCBULKCURRENTED		0x002cU				/**< Bulk Current ED	 */
#define F_USB20HO_HCDONEHEAD			0x0030U				/**< Done Head		 */
#define F_USB20HO_HCFMINTERVAL			0x0034U				/**< Fm Interval	 */
#define F_USB20HO_HCFMREMAINING 		0x0038U				/**< Fm Remaining	 */
#define F_USB20HO_HCFMNUMBER			0x003cU				/**< Fm Number		 */
#define F_USB20HO_HCPERIODICSTART		0x0040U				/**< Periodic Start	 */
#define F_USB20HO_HCLSTHRESHOLD 		0x0044U				/**< LS Threshold	 */
#define F_USB20HO_HCRHDESCRIPTORA		0x0048U				/**< Rh Descriptor A	 */
#define F_USB20HO_HCRHDESCRIPTORB		0x004cU				/**< Rh Descriptor B	 */
#define F_USB20HO_HCRHSTATUS			0x0050U				/**< Rh Status		 */
#define F_USB20HO_HCRHPORTSTATUS		0x0054U				/**< Rh Port Status	 */

/* Other Registers */
#define F_USB20HO_OTHER_BASE			0x2000U				/**< Other Registers Base */
#define F_USB20HO_LINKMODESETTING		(F_USB20HO_OTHER_BASE + 0x0000)	/**< Link Mode Setting	  */
#define F_USB20HO_PHYMODESETTING1		(F_USB20HO_OTHER_BASE + 0x0004U)/**< PHY Mode Setting1	  */
#define F_USB20HO_RESET 			(1U << 31U)			/**< */
#define F_USB20HO_PHYMODESETTING2		(F_USB20HO_OTHER_BASE + 0x0008U)/**< PHY Mode Setting2	  */

/* FVC2_H2XBB Registers */
#define F_USB20HO_FVC2_H2XBB_BASE		0x0000				/**< FVC2_H2XBB Registers Base */
#define F_USB20HO_FVC2_H2XBB_OP 		0x0000
#define F_USB20HO_BVALIDMODE			(1U << 0) 			/**< BVALIDMODE 	       */
#define F_USB20HO_REMAIN_MONITOR		(1U << 8U)			/**< REMAIN MONITOR	       */

static inline void set_f_usb20ho_register(void __iomem * base_address, unsigned long offset_address, unsigned long value)
{
	__raw_writel(value, base_address + offset_address);
	return;
}

static inline unsigned long get_f_usb20ho_register(void __iomem * base_address, unsigned long offset_address)
{
	return __raw_readl(base_address + offset_address);
}

/* So far, it is implementation of F_USB20HO controller register				*/
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HO driver configuration function				*/

static inline void __iomem *remap_iomem_region(unsigned long base_address, unsigned long size)
{
	/* unused */
	(void) size;

	return (void __iomem *) IO_ADDRESS(base_address);
}

static inline void unmap_iomem_region(void __iomem * base_address)
{
	/* unused */
	(void) base_address;

	return;
}

static inline void set_bus_power_supply(void __iomem * base_address)
{
	/* unused */
	(void) base_address;

	return;
}

static inline void enable_bus_power_supply(void __iomem * base_address, unsigned char enable)
{
	/* unused */
	(void) base_address;

	return;
}

/**
 * @brief g_fvc2_h2xbb_regs
 *
 * FVC2_H2XBB Registers
 */
static void __iomem *g_fvc2_h2xbb_regs = NULL;

static inline void fvc2_h2xbb_complete_wait(void)
{
	void __iomem * base_address = g_fvc2_h2xbb_regs;
	unsigned long count;

	set_f_usb20ho_register(base_address, F_USB20HO_FVC2_H2XBB_OP, F_USB20HO_BVALIDMODE);

	for(count = 0xFFFFFFFFU ; count && !(F_USB20HO_REMAIN_MONITOR & get_f_usb20ho_register(base_address, F_USB20HO_FVC2_H2XBB_OP)) ; count--){}

	return;
}


#define f_usb_roundup_32(len)	(((unsigned long)len + 31UL) & ~31UL)	/** 32 byte alignment macro */
#define SG_NENTS_MAX	256						/** L2 Cache Size alignment buf limmit */

/**
 * @brief fusb_temp_buf
 *
 * L2 Cache Size alignment buffer
 */
static size_t fusb_temp_buf[SG_NENTS_MAX];

static inline dma_addr_t f_usb20ho_dma_map_single(struct device *dev, void *cpu_addr,
		size_t size, enum dma_data_direction dir)
{
	return dma_map_single(dev, cpu_addr, (size_t)f_usb_roundup_32(size), dir);
}

static inline void f_usb20ho_dma_unmap_single(struct device *dev, dma_addr_t handle,
		size_t size, enum dma_data_direction dir)
{
	dma_unmap_single(dev, handle, (size_t)f_usb_roundup_32(size), dir);
}

static inline void f_usb20ho_sg_length_backup(struct scatterlist *sg, int nents, size_t *pbuf)
{
	struct scatterlist *s;
	size_t *plength = pbuf;
	int i;
	
	if( plength == NULL )
		return;
	
	for_each_sg(sg, s, nents, i) {
		*plength  = s->length;
		s->length = (size_t)f_usb_roundup_32(s->length);
		plength++;
	}
	return;
}

static inline void f_usb20ho_sg_length_writeback(struct scatterlist *sg, int nents, size_t *pbuf)
{
	struct scatterlist *s;
	size_t *plength = pbuf;
	int i;
	
	if( plength == NULL )
		return;
	
	for_each_sg(sg, s, nents, i) {
		s->length = *plength;
		plength++;
	}
	return;
}

static inline int f_usb20ho_dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	int ret = 0;
	void *pmalloc = NULL;
	size_t *pbuf;
	
	/* nents check */
	if( nents > SG_NENTS_MAX ){
		/* work buf malloc */
		pmalloc = kmalloc(sizeof(size_t) * nents, GFP_KERNEL);
		pbuf = (size_t *)pmalloc;
	}else{
		pbuf = fusb_temp_buf;
	}

	/* length backup */
	if( pbuf != NULL )
		f_usb20ho_sg_length_backup(sg, nents, pbuf);
	
	/* DMA Mapping */
	ret = dma_map_sg(dev, sg, nents, dir);
	
	/* length Write back */
	if( pbuf != NULL )
		f_usb20ho_sg_length_writeback(sg, nents, pbuf);
	
	if( pmalloc != NULL )
		kfree( pmalloc );
	
	return ret;
}

static inline void f_usb20ho_dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	void *pmalloc = NULL;
	size_t *pbuf;
	
	/* nents check */
	if( nents > SG_NENTS_MAX ){
		/* work buf malloc */
		pmalloc = kmalloc(sizeof(size_t) * nents, GFP_KERNEL);
		pbuf = (size_t *)pmalloc;
	}else{
		pbuf = fusb_temp_buf;
	}

	/* length backup */
	if( pbuf != NULL )
		f_usb20ho_sg_length_backup(sg, nents, pbuf);
	
	/* DMA UnMapping */
	dma_unmap_sg(dev, sg, nents, dir);
	
	/* length Write back */
	if( pbuf != NULL )
		f_usb20ho_sg_length_writeback(sg, nents, pbuf);
	
	if( pmalloc != NULL )
		kfree( pmalloc );
	
	return;
}

static inline void f_usb20ho_unmap_urb_setup_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	if (urb->transfer_flags & URB_SETUP_MAP_SINGLE)
		f_usb20ho_dma_unmap_single(hcd->self.controller,
				urb->setup_dma,
				sizeof(struct usb_ctrlrequest),
				DMA_TO_DEVICE);
	else if (urb->transfer_flags & URB_SETUP_MAP_LOCAL)
		;

	/* Make it safe to call this routine more than once */
	urb->transfer_flags &= ~(URB_SETUP_MAP_SINGLE | URB_SETUP_MAP_LOCAL);
	
	return;
}

static void inline f_usb20ho_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	enum dma_data_direction dir;

	f_usb20ho_unmap_urb_setup_for_dma(hcd, urb);

	dir = usb_urb_dir_in(urb) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
	if (urb->transfer_flags & URB_DMA_MAP_SG)
		f_usb20ho_dma_unmap_sg(hcd->self.controller,
				urb->sg,
				urb->num_sgs,
				dir);
	else if (urb->transfer_flags & URB_DMA_MAP_PAGE)
		dma_unmap_page(hcd->self.controller,
				urb->transfer_dma,
				(size_t)f_usb_roundup_32(urb->transfer_buffer_length),
				dir);
	else if (urb->transfer_flags & URB_DMA_MAP_SINGLE)
		f_usb20ho_dma_unmap_single(hcd->self.controller,
				urb->transfer_dma,
				urb->transfer_buffer_length,
				dir);
	else if (urb->transfer_flags & URB_MAP_LOCAL)
		;

	/* Make it safe to call this routine more than once */
	urb->transfer_flags &= ~(URB_DMA_MAP_SG | URB_DMA_MAP_PAGE |
			URB_DMA_MAP_SINGLE | URB_MAP_LOCAL);
}

static inline int f_usb20ho_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
			    gfp_t mem_flags)
{
	enum dma_data_direction dir;
	int ret = 0;

	/* Map the URB's buffers for DMA access.
	 * Lower level HCD code should use *_dma exclusively,
	 * unless it uses pio or talks to another transport,
	 * or uses the provided scatter gather list for bulk.
	 */

	if (usb_endpoint_xfer_control(&urb->ep->desc)) {
		if (hcd->self.uses_pio_for_control)
			return ret;
		if (hcd->self.uses_dma) {
			urb->setup_dma = f_usb20ho_dma_map_single(
					hcd->self.controller,
					urb->setup_packet,
					sizeof(struct usb_ctrlrequest),
					DMA_TO_DEVICE);
			if (dma_mapping_error(hcd->self.controller,
						urb->setup_dma))
				return -EAGAIN;
			urb->transfer_flags |= URB_SETUP_MAP_SINGLE;
		} else if (hcd->driver->flags & HCD_LOCAL_MEM) {
		}
	}

	dir = usb_urb_dir_in(urb) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
	if (urb->transfer_buffer_length != 0
	    && !(urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP)) {
		if (hcd->self.uses_dma) {
			if (urb->num_sgs) {
				int n = f_usb20ho_dma_map_sg(
						hcd->self.controller,
						urb->sg,
						urb->num_sgs,
						dir);
				if (n <= 0)
					ret = -EAGAIN;
				else
					urb->transfer_flags |= URB_DMA_MAP_SG;
				urb->num_mapped_sgs = n;
				if (n != urb->num_sgs)
					urb->transfer_flags |=
							URB_DMA_SG_COMBINED;
			} else if (urb->sg) {
				struct scatterlist *sg = urb->sg;
				urb->transfer_dma = dma_map_page(
						hcd->self.controller,
						sg_page(sg),
						sg->offset,
						(size_t)f_usb_roundup_32(urb->transfer_buffer_length),
						dir);
				if (dma_mapping_error(hcd->self.controller,
						urb->transfer_dma))
					ret = -EAGAIN;
				else
					urb->transfer_flags |= URB_DMA_MAP_PAGE;
			} else {
				urb->transfer_dma = f_usb20ho_dma_map_single(
						hcd->self.controller,
						urb->transfer_buffer,
						urb->transfer_buffer_length,
						dir);
				if (dma_mapping_error(hcd->self.controller,
						urb->transfer_dma))
					ret = -EAGAIN;
				else
					urb->transfer_flags |= URB_DMA_MAP_SINGLE;
			}
		} else if (hcd->driver->flags & HCD_LOCAL_MEM) {
		}
		if (ret && (urb->transfer_flags & (URB_SETUP_MAP_SINGLE |
				URB_SETUP_MAP_LOCAL)))
			f_usb20ho_unmap_urb_for_dma(hcd, urb);
	}
	return ret;
}

/* So far, it is implementation of F_USB20HO driver configuration function            */
/************************************************************************************************/

#endif
