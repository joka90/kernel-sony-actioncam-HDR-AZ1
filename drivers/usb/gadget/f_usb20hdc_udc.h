/*
 * drivers/usb/gadget/f_usb20hdc_udc.h
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

#ifndef _F_USB20HDC_UDC_H
#define _F_USB20HDC_UDC_H

/*
 * [notice]:There is a configuration function at the bottom
 */
#define EP_REDUCED

/************************************************************************************************/
/* Implementation of here to F_USB20HDC UDC driver configuration definition and parameter	*/

/* F_USB20HDC driver version */
#define F_USB20HDC_UDC_CONFIG_DRIVER_VERSION			"1.0.5"		/**< F_USB20HDC driver version */

/* F_USB20HDC C_HSEL area address offset */
#define F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET	0x00000		/**< F_USB20HDC C_HSEL area address offset */

/* F_USB20HDC D_HSEL area address offset */

#define F_USB20HDC_UDC_CONFIG_D_HSEL_AREA_ADDRESS_OFFSET	0x10000U	/**< F_USB20HDC D_HSEL area address offset */

/* endpoint counter count per one endpoint */
#define F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT	2U	/**< endpoint counter count per one endpoint */

/* endpoint channel count */
#ifdef EP_REDUCED
#define F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS				6U	/**< endpoint channel count */
#else
#define F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS				12U	/**< endpoint channel count */
#endif

/* endpoint buffer RAM size */
#define F_USB20HDC_UDC_CONFIG_ENDPOINT_BUFFER_RAM_SIZE		24576U		/**< endpoint buffer RAM size x1[bytes] */

/* F_USB20HDC USB controller DMA channel count */
#define F_USB20HDC_UDC_CONFIG_USB_DMA_CHANNELS			2U		/**< F_USB20HDC USB controller DMA channel count */

/* F_USB20HDC DMA controller DMA channel count */
#define F_USB20HDC_UDC_CONFIG_DMA_CHANNELS			2U		/**< F_USB20HDC DMA controller DMA channel count */

/* F_USB20HDC DMA controller transfer maximum byte */
#define F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES	4194304U	/**< F_USB20HDC DMA controller transfer maximum byte x1[bytes] */

#define F_USBHDC_C_INTR_IRQ		227U				/**< USB C_INTR */
#define F_USBHDC_DIRQ0_IRQ		228U				/**< HDMAC DIRQ0 */
#define F_USBHDC_DIRQ1_IRQ		229U				/**< HDMAC DIRQ1 */

int F_USB20HDC_DMA_IRQ[F_USB20HDC_UDC_CONFIG_DMA_CHANNELS] = {
	F_USBHDC_DIRQ0_IRQ,			/* HDMAC DIRQ0 */
	F_USBHDC_DIRQ1_IRQ,			/* HDMAC DIRQ1 */
};

/**
 * @brief endpoint configuration structure array table
 *
*/
static const struct {
	char *name;
	unsigned short	maximum_packet_size_high_speed;
	/**<
	 * endpoint transfer maximum packet size for high-speed
	 * [notice]:unusing it is referred to as '0'
	 */
	unsigned short	maximum_packet_size_full_speed;
	/**<
	 * endpoint transfer maximum packet size for full-speed
	 * [notice]:unusing it is referred to as '0'
	 */
	unsigned short	buffer_size;
	/**<
	 * endpoint buffer size(x1[bytes])
	 * [notice]:unusing it is referred to as '0'
	 */
	unsigned char	buffers;
	/**<
	 * endpoint buffer count
	 * [notice]:unusing it is referred to as '0', and endpoint 0 is 1 fixation
	 */
	unsigned char	pio_transfer_auto_change;
	/**<
	 * PIO transfer auto change flag
	 * [notice]:unusing it is referred to as '0', and endpoint 0 is 0 fixation
	 */
	unsigned char	in_transfer_end_timinig_host_transfer;
	/**<
	 * IN transfer end notify timing to USB host flag
	 * [notice]:unusing it is referred to as '0', and endpoint 0 is 0 fixation
	 */
	signed char	usb_dma_channel;
	/**<
	 * USB controller DMA channel for endpoint
	 * [notice]:unusing it is referred to as '-1'
	 */
	signed char	dma_channel;
	/**<
	 * DMA controller DMA channel for endpoint
	 * [notice]:unusing it is referred to as '-1'
	 */
	unsigned char	dma_request_channel;
	/**<
	 * DMA controller DMA request channel for endpoint
	 * [notice]:unusing it is referred to as '0'
	 */

#define DMA_REQUEST_NONE	0	/**< none */
#define DMA_REQUEST_DREQ_HIGH	1U	/**< DREQ High level or Positive edge */
#define DMA_REQUEST_IDREQ0	2U	/**< IDREQ[0] High level or Positive edge */
#define DMA_REQUEST_IDREQ1	3U	/**< IDREQ[1] High level or Positive edge */
#define DMA_REQUEST_IDREQ2	4U	/**< IDREQ[2] High level or Positive edge */
#define DMA_REQUEST_IDREQ3	5U	/**< IDREQ[3] High level or Positive edge */
#define DMA_REQUEST_IDREQ4	6U	/**< IDREQ[4] High level or Positive edge */
#define DMA_REQUEST_IDREQ5	7U	/**< IDREQ[5] High level or Positive edge */
#define DMA_REQUEST_IDREQ6	8U	/**< IDREQ[6] High level or Positive edge */
#define DMA_REQUEST_IDREQ7	9U	/**< IDREQ[7] High level or Positive edge */
#define DMA_REQUEST_IDREQ8	10U	/**< IDREQ[8] High level or Positive edge */
#define DMA_REQUEST_IDREQ9	11U	/**< IDREQ[9] High level or Positive edge */
#define DMA_REQUEST_IDREQ10	12U	/**< IDREQ[10] High level or Positive edge */
#define DMA_REQUEST_IDREQ11	13U	/**< IDREQ[11] High level or Positive edge */
#define DMA_REQUEST_IDREQ12	14U	/**< IDREQ[12] High level or Positive edge */
#define DMA_REQUEST_IDREQ13	15U	/**< IDREQ[13] High level or Positive edge */
#define DMA_REQUEST_IDREQ14	16U	/**< IDREQ[14] High level or Positive edge */
#define DMA_REQUEST_IDREQ15	17U	/**< IDREQ[15] High level or Positive edge */
} endpoint_configuration_data[F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS] = {
	/* endpoint 0 */
	[0] = {
		.name					= "ep0",
		.maximum_packet_size_high_speed 	= 64U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 64U,
		.buffers				= 1U,
		.pio_transfer_auto_change		= 0,
		.in_transfer_end_timinig_host_transfer	= 0,
		.usb_dma_channel			= -1,
		.dma_channel				= -1,
		.dma_request_channel			= DMA_REQUEST_NONE,
	},
	/* endpoint 1 */
	[1] = {
		.name					= "ep1-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 1U,
		.dma_channel				= 1U,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 2 */
	[2] = {
		.name					= "ep2-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 0,
		.dma_channel				= 0,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 3 */
	[3] = {
		.name					= "ep3-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 1U,
		.dma_channel				= 1U,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 4 */
	[4] = {
		.name					= "ep4-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 0,
		.dma_channel				= 0,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 5 */
	[5] = {
		.name					= "ep5-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 1U,
		.dma_channel				= 1U,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
#ifndef EP_REDUCED
	/* endpoint 6 */
	[6] = {
		.name					= "ep6-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 0,
		.dma_channel				= 0,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 7 */
	[7] = {
		.name					= "ep7-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 1U,
		.dma_channel				= 1U,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 8 */
	[8] = {
		.name					= "ep8-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 0,
		.dma_channel				= 0,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 9 */
	[9] = {
		.name					= "ep9-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 1U,
		.dma_channel				= 1U,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 10 */
	[10] = {
		.name					= "ep10-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 0,
		.dma_channel				= 0,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
	/* endpoint 11 */
	[11] = {
		.name					= "ep11-bulk",
		.maximum_packet_size_high_speed 	= 1024U,
		.maximum_packet_size_full_speed 	= 64U,
		.buffer_size				= 1024U,
		.buffers				= 2U,
		.pio_transfer_auto_change		= 1U,
		.in_transfer_end_timinig_host_transfer	= 1U,
		.usb_dma_channel			= 1U,
		.dma_channel				= 1U,
		.dma_request_channel			= DMA_REQUEST_DREQ_HIGH
	},
#endif
};

/* So far, it is implementation of F_USB20HDC UDC driver configuration definition and parameter	*/
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HDC controller register                     */

/* F_USB20HDC controller register ID Enumeration constant */
#define F_USB20HDC_REGISTER_CONF		0				/**< System Configuration */
#define F_USB20HDC_REGISTER_MODE		1U				/**< Oepration Mode */
#define F_USB20HDC_REGISTER_INTEN		2U				/**< Global Interrupt Enable */
#define F_USB20HDC_REGISTER_INTS		3U				/**< Global Interrupt Status */
#define F_USB20HDC_REGISTER_EPCMD0		4U				/**< EP0 Command */
#define F_USB20HDC_REGISTER_EPCMD1		5U				/**< EP1 Command */
#define F_USB20HDC_REGISTER_EPCMD2		6U				/**< EP2 Command */
#define F_USB20HDC_REGISTER_EPCMD3		7U				/**< EP3 Command */
#define F_USB20HDC_REGISTER_EPCMD4		8U				/**< EP4 Command */
#define F_USB20HDC_REGISTER_EPCMD5		9U				/**< EP5 Command */
#define F_USB20HDC_REGISTER_EPCMD6		10U				/**< EP6 Command */
#define F_USB20HDC_REGISTER_EPCMD7		11U				/**< EP7 Command */
#define F_USB20HDC_REGISTER_EPCMD8		12U				/**< EP8 Command */
#define F_USB20HDC_REGISTER_EPCMD9		13U				/**< EP9 Command */
#define F_USB20HDC_REGISTER_EPCMD10		14U				/**< EP10 Command */
#define F_USB20HDC_REGISTER_EPCMD11		15U				/**< EP11 Command */
#define F_USB20HDC_REGISTER_EPCMD12		16U				/**< EP12 Command */
#define F_USB20HDC_REGISTER_EPCMD13		17U				/**< EP13 Command */
#define F_USB20HDC_REGISTER_EPCMD14		18U				/**< EP14 Command */
#define F_USB20HDC_REGISTER_EPCMD15		19U				/**< EP15 Command */
#define F_USB20HDC_REGISTER_DEVC		20U				/**< Device Control */
#define F_USB20HDC_REGISTER_DEVS		21U				/**< Device Status */
#define F_USB20HDC_REGISTER_FADDR		22U				/**< Function Address */
#define F_USB20HDC_REGISTER_TSTAMP		23U				/**< Device Time Stamp */
#define F_USB20HDC_REGISTER_OTGC		24U				/**< OTG Control */
#define F_USB20HDC_REGISTER_OTGSTS		25U				/**< OTG Status */
#define F_USB20HDC_REGISTER_OTGSTSC		26U				/**< OTG Status Change */
#define F_USB20HDC_REGISTER_OTGSTSFALL		27U				/**< OTG Status Fall Detect Enabel */
#define F_USB20HDC_REGISTER_OTGSTSRISE		28U				/**< OTG Status Rise Detect Enabel */
#define F_USB20HDC_REGISTER_OTGTC		29U				/**< OTG Timer Control */
#define F_USB20HDC_REGISTER_OTGT		30U				/**< OTG Timer */
#define F_USB20HDC_REGISTER_DMAC1		31U				/**< DMA1 Control */
#define F_USB20HDC_REGISTER_DMAS1		32U				/**< DMA1 Status */
#define F_USB20HDC_REGISTER_DMATCI1		33U				/**< DMA1 Continuously Transfer Number-of-bytes Setting */
#define F_USB20HDC_REGISTER_DMATC1		34U				/**< DMA1 Continuously Transfer Bytes Counter */
#define F_USB20HDC_REGISTER_DMAC2		35U				/**< DMA2 Control */
#define F_USB20HDC_REGISTER_DMAS2		36U				/**< DMA2 Status */
#define F_USB20HDC_REGISTER_DMATCI2		37U				/**< DMA2 Continuously Transfer Number-of-bytes Setting */
#define F_USB20HDC_REGISTER_DMATC2		38U				/**< DMA2 Continuously Transfer Bytes Counter */
#define F_USB20HDC_REGISTER_TESTC		39U				/**< Test Control */
#define F_USB20HDC_REGISTER_EPCTRL0		40U				/**< EP0 Control */
#define F_USB20HDC_REGISTER_EPCTRL1		41U				/**< EP1 Control */
#define F_USB20HDC_REGISTER_EPCTRL2		42U				/**< EP2 Control */
#define F_USB20HDC_REGISTER_EPCTRL3		43U				/**< EP3 Control */
#define F_USB20HDC_REGISTER_EPCTRL4		44U				/**< EP4 Control */
#define F_USB20HDC_REGISTER_EPCTRL5		45U				/**< EP5 Control */
#define F_USB20HDC_REGISTER_EPCTRL6		46U				/**< EP6 Control */
#define F_USB20HDC_REGISTER_EPCTRL7		47U				/**< EP7 Control */
#define F_USB20HDC_REGISTER_EPCTRL8		48U				/**< EP8 Control */
#define F_USB20HDC_REGISTER_EPCTRL9		49U				/**< EP9 Control */
#define F_USB20HDC_REGISTER_EPCTRL10		50U				/**< EP10 Control */
#define F_USB20HDC_REGISTER_EPCTRL11		51U				/**< EP11 Control */
#define F_USB20HDC_REGISTER_EPCTRL12		52U				/**< EP12 Control */
#define F_USB20HDC_REGISTER_EPCTRL13		53U				/**< EP13 Control */
#define F_USB20HDC_REGISTER_EPCTRL14		54U				/**< EP14 Control */
#define F_USB20HDC_REGISTER_EPCTRL15		55U				/**< EP15 Control */
#define F_USB20HDC_REGISTER_EPCONF0		56U				/**< EP0 Config */
#define F_USB20HDC_REGISTER_EPCONF1		57U				/**< EP1 Config */
#define F_USB20HDC_REGISTER_EPCONF2		58U				/**< EP2 Config */
#define F_USB20HDC_REGISTER_EPCONF3		59U				/**< EP3 Config */
#define F_USB20HDC_REGISTER_EPCONF4		60U				/**< EP4 Config */
#define F_USB20HDC_REGISTER_EPCONF5		61U				/**< EP5 Config */
#define F_USB20HDC_REGISTER_EPCONF6		62U				/**< EP6 Config */
#define F_USB20HDC_REGISTER_EPCONF7		63U				/**< EP7 Config */
#define F_USB20HDC_REGISTER_EPCONF8		64U				/**< EP8 Config */
#define F_USB20HDC_REGISTER_EPCONF9		65U				/**< EP9 Config */
#define F_USB20HDC_REGISTER_EPCONF10		66U				/**< EP10 Config */
#define F_USB20HDC_REGISTER_EPCONF11		67U				/**< EP11 Config */
#define F_USB20HDC_REGISTER_EPCONF12		68U				/**< EP12 Config */
#define F_USB20HDC_REGISTER_EPCONF13		69U				/**< EP13 Config */
#define F_USB20HDC_REGISTER_EPCONF14		70U				/**< EP14 Config */
#define F_USB20HDC_REGISTER_EPCONF15		71U				/**< EP15 Config */
#define F_USB20HDC_REGISTER_EPCOUNT0		72U				/**< EP0 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT1		73U				/**< EP1 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT2		74U				/**< EP2 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT3		75U				/**< EP3 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT4		76U				/**< EP4 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT5		77U				/**< EP5 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT6		78U				/**< EP6 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT7		79U				/**< EP7 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT8		80U				/**< EP8 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT9		81U				/**< EP9 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT10		82U				/**< EP10 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT11		83U				/**< EP11 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT12		84U				/**< EP12 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT13		85U				/**< EP13 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT14		86U				/**< EP14 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT15		87U				/**< EP15 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT16		88U				/**< EP16 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT17		89U				/**< EP17 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT18		90U				/**< EP18 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT19		91U				/**< EP19 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT20		92U				/**< EP20 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT21		93U				/**< EP21 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT22		94U				/**< EP22 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT23		95U				/**< EP23 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT24		96U				/**< EP24 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT25		97U				/**< EP25 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT26		98U				/**< EP26 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT27		99U				/**< EP27 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT28		100U				/**< EP28 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT29		101U				/**< EP29 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT30		102U				/**< EP30 Counter */
#define F_USB20HDC_REGISTER_EPCOUNT31		103U				/**< EP31 Counter */
#define F_USB20HDC_REGISTER_EPBUF		104U				/**< EP Buffer	*/
#define F_USB20HDC_REGISTER_DMA1		105U				/**< DMA 1 Address Convert Area  */
#define F_USB20HDC_REGISTER_DMA2		106U				/**< DMA 1 Address Convert Area  */
#define F_USB20HDC_REGISTER_MAX 		107U				/**< Max Value */

/**
 * @brief dmac_register[]
 *
 * DMAx Control register ID table array
 */
static const unsigned long dmac_register[] = {
	F_USB20HDC_REGISTER_DMAC1,	/* DMA1 Control */
	F_USB20HDC_REGISTER_DMAC2,	/* DMA1 Control */
};

/**
 * @brief dmas_register[]
 *
 * DMAx Status register ID table array
 */
static const unsigned long dmas_register[] = {
	F_USB20HDC_REGISTER_DMAS1,	/* DMA1 Status */
	F_USB20HDC_REGISTER_DMAS2,	/* DMA1 Status */
};

/**
 * @brief dmatci_register[]
 *
 * DMAx Continuously Transfer Number-of-bytes Setting register ID table array
 */
static const unsigned long dmatci_register[] = {
	F_USB20HDC_REGISTER_DMATCI1,	/* DMA1 Continuously Transfer Number-of-bytes Setting */
	F_USB20HDC_REGISTER_DMATCI2,	/* DMA2 Continuously Transfer Number-of-bytes Setting */
};

/**
 * @brief dmatc_register[]
 *
 * DMAx Continuously Transfer Bytes Counter register ID table array
 */
static const unsigned long dmatc_register[] = {
	F_USB20HDC_REGISTER_DMATC1,	/* DMA1 Continuously Transfer Bytes Counter */
	F_USB20HDC_REGISTER_DMATC2,	/* DMA2 Continuously Transfer Bytes Counter */
};

/**
 * @brief F_USB20HDC controller register structures array
*/
static const struct {
	volatile unsigned long address_offset;					/**< register address offset */
	volatile unsigned char readable;					/**< register readable flag  */
	volatile unsigned char writable;					/**< register writable flag  */
} f_usb20hdc_register[F_USB20HDC_REGISTER_MAX] = {
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0000, 1U, 1U},	/* F_USB20HDC_REGISTER_CONF */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0004U, 1U, 1U},	/* F_USB20HDC_REGISTER_MODE */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0008U, 1U, 1U},	/* F_USB20HDC_REGISTER_INTEN */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x000cU, 1U, 1U},	/* F_USB20HDC_REGISTER_INTS */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0040U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD0 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0044U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0048U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD2 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x004cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD3 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0050U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD4 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0054U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD5 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0058U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD6 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x005cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD7 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0060U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD8 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0064U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD9 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0068U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD10 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x006cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD11 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0070U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD12 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0074U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD13 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0078U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD14 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x007cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCMD15 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0200U, 1U, 1U},	/* F_USB20HDC_REGISTER_DEVC */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0204U, 1U, 1U},	/* F_USB20HDC_REGISTER_DEVS */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0208U, 1U, 1U},	/* F_USB20HDC_REGISTER_FADDR */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x020cU, 1U, 0U},	/* F_USB20HDC_REGISTER_TSTAMP */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0300U, 1U, 1U},	/* F_USB20HDC_REGISTER_OTGC */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0310U, 1U, 0U},	/* F_USB20HDC_REGISTER_OTGSTS */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0314U, 1U, 1U},	/* F_USB20HDC_REGISTER_OTGSTSC */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0318U, 1U, 1U},	/* F_USB20HDC_REGISTER_OTGSTSFALL */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x031cU, 1U, 1U},	/* F_USB20HDC_REGISTER_OTGSTSRISE */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0320U, 1U, 1U},	/* F_USB20HDC_REGISTER_OTGTC */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0324U, 1U, 1U},	/* F_USB20HDC_REGISTER_OTGT */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0400U, 1U, 1U},	/* F_USB20HDC_REGISTER_DMAC1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0404U, 1U, 0U},	/* F_USB20HDC_REGISTER_DMAS1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0408U, 1U, 1U},	/* F_USB20HDC_REGISTER_DMATCI1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x040cU, 1U, 0U},	/* F_USB20HDC_REGISTER_DMATC1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0420U, 1U, 1U},	/* F_USB20HDC_REGISTER_DMAC2 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0424U, 1U, 0U},	/* F_USB20HDC_REGISTER_DMAS2 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0428U, 1U, 1U},	/* F_USB20HDC_REGISTER_DMATCI2 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x042cU, 1U, 0U},	/* F_USB20HDC_REGISTER_DMATC2 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x0500U, 1U, 1U},	/* F_USB20HDC_REGISTER_TESTC */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8000U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL0 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8004U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8008U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL2 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x800cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL3 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8010U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL4 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8014U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL5 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8018U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL6 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x801cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL7 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8020U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL8 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8024U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL9 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8028U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL10 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x802cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL11 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8030U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL12 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8034U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL13 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8038U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL14 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x803cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCTRL15 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8040U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF0 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8044U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8048U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF2 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x804cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF3 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8050U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF4 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8054U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF5 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8058U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF6 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x805cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF7 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8060U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF8 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8064U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF9 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8068U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF10 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x806cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF11 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8070U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF12 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8074U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF13 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8078U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF14 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x807cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCONF15 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8080U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT0 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8084U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8088U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT2 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x808cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT3 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8090U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT4 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8094U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT5 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8098U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT6 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x809cU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT7 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80a0U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT8 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80a4U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT9 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80a8U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT10 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80acU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT11 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80b0U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT12 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80b4U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT13 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80b8U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT14 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80bcU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT15 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80c0U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT16 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80c4U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT17 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80c8U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT18 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80ccU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT19 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80d0U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT20 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80d4U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT21 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80d8U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT22 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80dcU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT23 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80e0U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT24 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80e4U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT25 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80e8U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT26 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80ecU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT27 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80f0U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT28 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80f4U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT29 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80f8U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT30 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x80fcU, 1U, 1U},	/* F_USB20HDC_REGISTER_EPCOUNT31 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + 0x8100U, 1U, 1U},	/* F_USB20HDC_REGISTER_EPBUF */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_D_HSEL_AREA_ADDRESS_OFFSET + 0x0000, 1U, 1U}, 	/* F_USB20HDC_REGISTER_DMA1 */
	{(volatile unsigned long)F_USB20HDC_UDC_CONFIG_D_HSEL_AREA_ADDRESS_OFFSET + 0x8000U, 1U, 1U},	/* F_USB20HDC_REGISTER_DMA2 */
};

static inline void control_default_register_cache_bits(void __iomem *base_address, unsigned long register_id, volatile unsigned long *register_cache)
{
	return;
}

static inline void control_ints_register_cache_bits(void __iomem *base_address, unsigned long register_id, volatile unsigned long *register_cache)
{
	/* Global Interrupt Status register bit feild position */
	#define F_USB20HDC_REGISTER_INTS_BIT_PHY_ERR_INT	3U		/**< phy_err_int */
	#define F_USB20HDC_REGISTER_INTS_BIT_CMD_INT		4U		/**< cmd_int */
	#define F_USB20HDC_REGISTER_INTS_BIT_DMA1_INT		8U		/**< dma1_int */
	#define F_USB20HDC_REGISTER_INTS_BIT_DMA2_INT		9U		/**< dma2_int */

	*register_cache |= (((unsigned long)1 << F_USB20HDC_REGISTER_INTS_BIT_PHY_ERR_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_INTS_BIT_CMD_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_INTS_BIT_DMA1_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_INTS_BIT_DMA2_INT));
	return;
}

static inline unsigned char get_et(void __iomem *, unsigned char);
static inline unsigned char get_dir(void __iomem *, unsigned char);
static inline unsigned char get_bnum(void __iomem *, unsigned char);
static inline unsigned char get_hiband(void __iomem *, unsigned char);
static inline unsigned char get_nullresp(void __iomem *, unsigned char);
static inline unsigned char get_nackresp(void __iomem *, unsigned char);
static inline unsigned char get_readyi_ready_inten(void __iomem *, unsigned char);
static inline unsigned char get_readyo_empty_inten(void __iomem *, unsigned char);
static inline unsigned char get_ping_inten(void __iomem *, unsigned char);
static inline unsigned char get_stalled_inten(void __iomem *, unsigned char);
static inline unsigned char get_nack_inten(void __iomem *, unsigned char);
static inline void control_epcmd_register_cache_bits(void __iomem *base_address, unsigned long register_id, volatile unsigned long *register_cache)
{
	/* EPx Command register bit feild position */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_START			0		/**< start */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_STOP			1U		/**< stop */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_INIT			2U		/**< init */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_BUFWR			3U		/**< bufwr */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_BUFRD			4U		/**< bufrd */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_STALL_SET 		5U		/**< stall_set */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_STALL_CLR 		6U		/**< stall_clr */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_TOGGLE_SET		7U		/**< toggle_set */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_TOGGLE_CLR		8U		/**< toggle_clr */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_NULLRESP			9U		/**< nullresp */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_NACKRESP			10U 		/**< nackresp */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_WRITE_EN			11U 		/**< write_en */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_READYI_READY_INTEN	12U 		/**< readyi_inten[Endpoint 0] / ready_inten[Endpoint 1-15] */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_READYO_EMPTY_INTEN	13U 		/**< readyo_inten[Endpoint 0] / empty_inten[Endpoint 1-15] */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_PING_INTEN		14U 		/**< ping_inten */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_STALLED_INTEN		15U 		/**< stalled_inten */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_NACK_INTEN		16U 		/**< nack_inten */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_READYI_READY_INT_CLR	18U 		/**< readyi_int_clr[Endpoint 0] / ready_int_clr[Endpoint 1-15] */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_READYO_EMPTY_INT_CLR	19U 		/**< readyo_int_clr[Endpoint 0] / ready_empty_clr[Endpoint 1-15] */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_PING_INT_CLR		20U 		/**< ping_int_clr */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_STALLED_INT_CLR		21U 		/**< stalled_int_clr */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_NACK_INT_CLR		22U 		/**< nack_int_clr */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_ET			23U 		/**< et */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_DIR			25U 		/**< dir */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_BNUM			26U 		/**< bnum */
	#define F_USB20HDC_REGISTER_EPCMD_BIT_HIBAND			28U 		/**< hiband */

	unsigned char endpoint_channel = register_id - F_USB20HDC_REGISTER_EPCMD0;

	*register_cache &= ~(((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_START) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_STOP) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_INIT) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_BUFWR) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_BUFRD) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_STALL_SET) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_STALL_CLR) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_TOGGLE_SET) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_TOGGLE_CLR) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_READYI_READY_INT_CLR) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_READYO_EMPTY_INT_CLR) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_PING_INT_CLR) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_STALLED_INT_CLR) |
			     ((unsigned long)1U << F_USB20HDC_REGISTER_EPCMD_BIT_NACK_INT_CLR));

	*register_cache |= (((unsigned long)(get_nullresp(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_NULLRESP)) |
			    ((unsigned long)(get_nackresp(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_NACKRESP)) |
			    (((unsigned long)1 << F_USB20HDC_REGISTER_EPCMD_BIT_WRITE_EN)) |
			    ((unsigned long)(get_readyi_ready_inten(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_READYI_READY_INTEN)) |
			    ((unsigned long)(get_readyo_empty_inten(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_READYO_EMPTY_INTEN)) |
			    ((unsigned long)(get_ping_inten(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_PING_INTEN)) |
			    ((unsigned long)(get_stalled_inten(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_STALLED_INTEN)) |
			    ((unsigned long)(get_nack_inten(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_NACK_INTEN)) |
			    ((unsigned long)(get_et(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_ET)) |
			    ((unsigned long)(get_dir(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_DIR)) |
			    ((unsigned long)(get_bnum(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_BNUM)) |
			    ((unsigned long)(get_hiband(base_address, endpoint_channel) << F_USB20HDC_REGISTER_EPCMD_BIT_HIBAND)));

	return;
}

static inline void control_devs_register_cache_bits(void __iomem *base_address, unsigned long register_id, volatile unsigned long *register_cache)
{
	/* Device Status register bit feild position */
	#define F_USB20HDC_REGISTER_DEVS_BIT_SUSPENDE_INT	24U		/**< suspende_int */
	#define F_USB20HDC_REGISTER_DEVS_BIT_SUSPENDB_INT	25U		/**< suspendb_int */
	#define F_USB20HDC_REGISTER_DEVS_BIT_SOF_INT		26U		/**< sof_int */
	#define F_USB20HDC_REGISTER_DEVS_BIT_SETUP_INT		27U		/**< setup_int */
	#define F_USB20HDC_REGISTER_DEVS_BIT_USBRSTE_INT	28U		/**< usbrste_int */
	#define F_USB20HDC_REGISTER_DEVS_BIT_USBRSTB_INT	29U		/**< usbrstb_int */
	#define F_USB20HDC_REGISTER_DEVS_BIT_STATUS_OK_INT	30U		/**< status_ok_int */
	#define F_USB20HDC_REGISTER_DEVS_BIT_STATUS_NG_INT	31U		/**< status_ng_int */

	*register_cache |= (((unsigned long)1 << F_USB20HDC_REGISTER_DEVS_BIT_SUSPENDE_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_DEVS_BIT_SUSPENDB_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_DEVS_BIT_SOF_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_DEVS_BIT_SETUP_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_DEVS_BIT_USBRSTE_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_DEVS_BIT_USBRSTB_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_DEVS_BIT_STATUS_OK_INT) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_DEVS_BIT_STATUS_NG_INT));
	return;
}

static inline void control_otgstsc_register_cache_bits(void __iomem *base_address, unsigned long register_id, volatile unsigned long *register_cache)
{
	/* OTG Status Change register bit feild position */
	#define F_USB20HDC_REGISTER_OTGSTS_BIT_OTG_TMROUT_C	0		/**< otg_tmrout_c */
	#define F_USB20HDC_REGISTER_OTGSTS_BIT_ID_C		6U		/**< id_c */
	#define F_USB20HDC_REGISTER_OTGSTS_BIT_VBUS_VLD_C	10U		/**< vbus_vld_c */

	*register_cache |= (((unsigned long)1 << F_USB20HDC_REGISTER_OTGSTS_BIT_OTG_TMROUT_C) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_OTGSTS_BIT_ID_C) |
			    ((unsigned long)1 << F_USB20HDC_REGISTER_OTGSTS_BIT_VBUS_VLD_C));
	return;
}

static inline void control_register_cache_bits(void __iomem *base_address, unsigned long register_id, volatile unsigned long *register_cache)
{
	static void (* const register_cache_bits_control_function[])(void __iomem *, unsigned long, volatile unsigned long *) = {
		control_default_register_cache_bits,		/* System Configuration */
		control_default_register_cache_bits,		/* Oepration Mode */
		control_default_register_cache_bits,		/* Global Interrupt Enable */
		control_ints_register_cache_bits,		/* Global Interrupt Status */
		control_epcmd_register_cache_bits,		/* EP0 Command */
		control_epcmd_register_cache_bits,		/* EP1 Command */
		control_epcmd_register_cache_bits,		/* EP2 Command */
		control_epcmd_register_cache_bits,		/* EP3 Command */
		control_epcmd_register_cache_bits,		/* EP4 Command */
		control_epcmd_register_cache_bits,		/* EP5 Command */
		control_epcmd_register_cache_bits,		/* EP6 Command */
		control_epcmd_register_cache_bits,		/* EP7 Command */
		control_epcmd_register_cache_bits,		/* EP8 Command */
		control_epcmd_register_cache_bits,		/* EP9 Command */
		control_epcmd_register_cache_bits,		/* EP10 Command */
		control_epcmd_register_cache_bits,		/* EP11 Command */
		control_epcmd_register_cache_bits,		/* EP12 Command */
		control_epcmd_register_cache_bits,		/* EP13 Command */
		control_epcmd_register_cache_bits,		/* EP14 Command */
		control_epcmd_register_cache_bits,		/* EP15 Command */
		control_default_register_cache_bits,		/* Device Control */
		control_devs_register_cache_bits,		/* Device Status */
		control_default_register_cache_bits,		/* Function Address */
		control_default_register_cache_bits,		/* Device Time Stamp */
		control_default_register_cache_bits,		/* OTG Control */
		control_default_register_cache_bits,		/* OTG Status */
		control_otgstsc_register_cache_bits,		/* OTG Status Change */
		control_default_register_cache_bits,		/* OTG Status Fall Detect Enabel */
		control_default_register_cache_bits,		/* OTG Status Rise Detect Enabel */
		control_default_register_cache_bits,		/* OTG Timer Control */
		control_default_register_cache_bits,		/* OTG Timer */
		control_default_register_cache_bits,		/* DMA1 Control */
		control_default_register_cache_bits,		/* DMA1 Status */
		control_default_register_cache_bits,		/* DMA1 Continuously Transfer Number-of-bytes Setting */
		control_default_register_cache_bits,		/* DMA1 Continuously Transfer Bytes Counter */
		control_default_register_cache_bits,		/* DMA2 Control */
		control_default_register_cache_bits,		/* DMA2 Status */
		control_default_register_cache_bits,		/* DMA2 Continuously Transfer Number-of-bytes Setting */
		control_default_register_cache_bits,		/* DMA2 Continuously Transfer Bytes Counter */
		control_default_register_cache_bits,		/* Test Control */
		control_default_register_cache_bits,		/* EP0 Control */
		control_default_register_cache_bits,		/* EP1 Control */
		control_default_register_cache_bits,		/* EP2 Control */
		control_default_register_cache_bits,		/* EP3 Control */
		control_default_register_cache_bits,		/* EP4 Control */
		control_default_register_cache_bits,		/* EP5 Control */
		control_default_register_cache_bits,		/* EP6 Control */
		control_default_register_cache_bits,		/* EP7 Control */
		control_default_register_cache_bits,		/* EP8 Control */
		control_default_register_cache_bits,		/* EP9 Control */
		control_default_register_cache_bits,		/* EP10 Control */
		control_default_register_cache_bits,		/* EP11 Control */
		control_default_register_cache_bits,		/* EP12 Control */
		control_default_register_cache_bits,		/* EP13 Control */
		control_default_register_cache_bits,		/* EP14 Control */
		control_default_register_cache_bits,		/* EP15 Control */
		control_default_register_cache_bits,		/* EP0 Config */
		control_default_register_cache_bits,		/* EP1 Config */
		control_default_register_cache_bits,		/* EP2 Config */
		control_default_register_cache_bits,		/* EP3 Config */
		control_default_register_cache_bits,		/* EP4 Config */
		control_default_register_cache_bits,		/* EP5 Config */
		control_default_register_cache_bits,		/* EP6 Config */
		control_default_register_cache_bits,		/* EP7 Config */
		control_default_register_cache_bits,		/* EP8 Config */
		control_default_register_cache_bits,		/* EP9 Config */
		control_default_register_cache_bits,		/* EP10 Config */
		control_default_register_cache_bits,		/* EP11 Config */
		control_default_register_cache_bits,		/* EP12 Config */
		control_default_register_cache_bits,		/* EP13 Config */
		control_default_register_cache_bits,		/* EP14 Config */
		control_default_register_cache_bits,		/* EP15 Config */
		control_default_register_cache_bits,		/* EP0 Counter */
		control_default_register_cache_bits,		/* EP1 Counter */
		control_default_register_cache_bits,		/* EP2 Counter */
		control_default_register_cache_bits,		/* EP3 Counter */
		control_default_register_cache_bits,		/* EP4 Counter */
		control_default_register_cache_bits,		/* EP5 Counter */
		control_default_register_cache_bits,		/* EP6 Counter */
		control_default_register_cache_bits,		/* EP7 Counter */
		control_default_register_cache_bits,		/* EP8 Counter */
		control_default_register_cache_bits,		/* EP9 Counter */
		control_default_register_cache_bits,		/* EP10 Counter */
		control_default_register_cache_bits,		/* EP11 Counter */
		control_default_register_cache_bits,		/* EP12 Counter */
		control_default_register_cache_bits,		/* EP13 Counter */
		control_default_register_cache_bits,		/* EP14 Counter */
		control_default_register_cache_bits,		/* EP15 Counter */
		control_default_register_cache_bits,		/* EP16 Counter */
		control_default_register_cache_bits,		/* EP17 Counter */
		control_default_register_cache_bits,		/* EP18 Counter */
		control_default_register_cache_bits,		/* EP19 Counter */
		control_default_register_cache_bits,		/* EP20 Counter */
		control_default_register_cache_bits,		/* EP21 Counter */
		control_default_register_cache_bits,		/* EP22 Counter */
		control_default_register_cache_bits,		/* EP23 Counter */
		control_default_register_cache_bits,		/* EP24 Counter */
		control_default_register_cache_bits,		/* EP25 Counter */
		control_default_register_cache_bits,		/* EP26 Counter */
		control_default_register_cache_bits,		/* EP27 Counter */
		control_default_register_cache_bits,		/* EP28 Counter */
		control_default_register_cache_bits,		/* EP29 Counter */
		control_default_register_cache_bits,		/* EP30 Counter */
		control_default_register_cache_bits,		/* EP31 Counter */
		control_default_register_cache_bits,		/* EP buffer */
	};
	register_cache_bits_control_function[register_id](base_address, register_id, register_cache);
	return;
}

static inline void set_register_bits(void __iomem *base_address, unsigned long register_id, unsigned char start_bit,
				     unsigned char bit_length, unsigned long value)
{
	volatile unsigned long register_cache = 0x00;
	unsigned long mask = (unsigned long)-1 >> (32U - bit_length);

	value &= mask;

	if (f_usb20hdc_register[register_id].readable)
		register_cache = __raw_readl(base_address + f_usb20hdc_register[register_id].address_offset);

	control_register_cache_bits(base_address, register_id, &register_cache);

	register_cache &= ~(mask << start_bit);
	register_cache |= (value << start_bit);

	if (f_usb20hdc_register[register_id].writable)
		__raw_writel(register_cache, base_address + f_usb20hdc_register[register_id].address_offset);

	return;
}

static inline unsigned long get_register_bits(void __iomem *base_address, unsigned long register_id, unsigned char start_bit,
		unsigned char bit_length)
{
	volatile unsigned long register_cache = 0x00;
	unsigned long mask = (unsigned long)-1 >> (32U - bit_length);

	if (f_usb20hdc_register[register_id].readable)
		register_cache = __raw_readl(base_address + f_usb20hdc_register[register_id].address_offset);

	return register_cache >> start_bit & mask;
}

static inline unsigned long get_epctrl_register_bits(void __iomem *base_address, unsigned long register_id, unsigned char start_bit,
		unsigned char bit_length)
{
	unsigned long counter;
	volatile unsigned long register_cache[3] = { [0 ... 2] = 0x00, };
	unsigned long mask = (unsigned long)-1 >> (32U - bit_length);

	if (f_usb20hdc_register[register_id].readable) {
		for (counter = 0xffffU; counter; counter--) {
			register_cache[0] = __raw_readl(base_address + f_usb20hdc_register[register_id].address_offset) >> start_bit & mask;
			register_cache[1] = __raw_readl(base_address + f_usb20hdc_register[register_id].address_offset) >> start_bit & mask;
			register_cache[2] = __raw_readl(base_address + f_usb20hdc_register[register_id].address_offset) >> start_bit & mask;
			if ((register_cache[0] == register_cache[1]) && (register_cache[1] == register_cache[2]))
				break;
		}
	}
	return register_cache[2];
}

static inline void set_byte_order(void __iomem *base_address, unsigned char big_endian)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_CONF, 0, 1U, big_endian);
	return;
}

static inline void set_burst_wait(void __iomem *base_address, unsigned char waits)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_CONF, 1U, 1U, waits);
	return;
}

static inline void set_soft_reset(void __iomem *base_address)
{
	unsigned long counter;
	volatile unsigned long register_temp = __raw_readl(base_address + 0x18);
	/* soft reset */
	set_register_bits(base_address, F_USB20HDC_REGISTER_CONF, 2U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_CONF, 2U, 1U))); counter--);
	/* phy setting */
	__raw_writel(register_temp, base_address + 0x18);
	return;
}

static inline void set_host_en(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_MODE, 0, 1U, enable);
	return;
}

static inline unsigned char get_host_en(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_MODE, 0, 1U);
}

static inline void set_dev_en(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_MODE, 1U, 1U, enable);
	return;
}

static inline unsigned char get_dev_en(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_MODE, 1U, 1U);
}

static inline void set_dev_int_mode(void __iomem *base_address, unsigned char mode)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_MODE, 2U, 1U, mode);
	return;
}

static inline void set_dev_addr_load_mode(void __iomem *base_address, unsigned char mode)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_MODE, 3U, 1U, mode);
	return;
}

static inline void set_host_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 0, 1U, enable);
	return;
}

static inline unsigned char get_host_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 0, 1U);
}

static inline void set_dev_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 1U, 1U, enable);
	return;
}

static inline unsigned char get_dev_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 1U, 1U);
}

static inline void set_otg_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 2U, 1U, enable);
	return;
}

static inline unsigned char get_otg_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 2U, 1U);
}

static inline void set_phy_err_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 3U, 1U, enable);
	return;
}

static inline unsigned char get_phy_err_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 3U, 1U);
}

static inline void set_cmd_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 4U, 1U, enable);
	return;
}

static inline unsigned char get_cmd_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 4U, 1U);
}

static inline void set_dma_inten(void __iomem *base_address, unsigned char dma_channel, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 8U + dma_channel, 1U, enable);
	return;
}

/* USB DMA channel symbolic constant */
#define USB_DMA_CHANNEL_DMA0	0	/**< USB DMA channel 0 */
#define USB_DMA_CHANNEL_DMA1	1U	/**< USB DMA channel 1 */

static inline unsigned char get_dma_inten(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 8U + dma_channel, 1U);
}

static inline void set_dev_ep_inten(void __iomem *base_address, unsigned char endpoint_channel, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 16U + endpoint_channel, 1U, enable);
	return;
}

/* endpoint channel symbolic constant */
#define ENDPOINT_CHANNEL_ENDPOINT0	0				/**< endpoint 0 */
#define ENDPOINT_CHANNEL_ENDPOINT1	1U				/**< endpoint 1 */
#define ENDPOINT_CHANNEL_ENDPOINT2	2U				/**< endpoint 2 */
#define ENDPOINT_CHANNEL_ENDPOINT3	3U				/**< endpoint 3 */
#define ENDPOINT_CHANNEL_ENDPOINT4	4U				/**< endpoint 4 */
#define ENDPOINT_CHANNEL_ENDPOINT5	5U				/**< endpoint 5 */
#define ENDPOINT_CHANNEL_ENDPOINT6	6U				/**< endpoint 6 */
#define ENDPOINT_CHANNEL_ENDPOINT7	7U				/**< endpoint 7 */
#define ENDPOINT_CHANNEL_ENDPOINT8	8U				/**< endpoint 8 */
#define ENDPOINT_CHANNEL_ENDPOINT9	9U				/**< endpoint 9 */
#define ENDPOINT_CHANNEL_ENDPOINT10	10U				/**< endpoint 10 */
#define ENDPOINT_CHANNEL_ENDPOINT11	11U				/**< endpoint 11 */
#define ENDPOINT_CHANNEL_ENDPOINT12	12U				/**< endpoint 12 */
#define ENDPOINT_CHANNEL_ENDPOINT13	13U				/**< endpoint 13 */
#define ENDPOINT_CHANNEL_ENDPOINT14	14U				/**< endpoint 14 */
#define ENDPOINT_CHANNEL_ENDPOINT15	15U				/**< endpoint 15 */

static inline unsigned char get_dev_ep_inten(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTEN, 16U + endpoint_channel, 1U);
}

static inline unsigned char get_host_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 0, 1U);
}

static inline unsigned char get_dev_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 1U, 1U);
}

static inline unsigned char get_otg_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 2U, 1U);
}

static inline unsigned char get_phy_err_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 3U, 1U);
}

static inline void clear_phy_err_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 3U, 1U, 0);
	return;
}

static inline unsigned char get_cmd_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 4U, 1U);
}

static inline void clear_cmd_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 4U, 1U, 0);
	return;
}

static inline unsigned char get_dma_int(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 8U + dma_channel, 1U);
}

static inline void clear_dma_int(void __iomem *base_address, unsigned char dma_channel)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 8U + dma_channel, 1U, 0);
	return;
}

static inline unsigned char get_dev_ep_int(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_INTS, 16U + endpoint_channel, 1U);
}

static inline void set_start(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffff; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 0, 1U, 1U);
	for (counter = 0xffff; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_stop(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 1U, 1U, 1U);
	get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 17U, 1U) ? udelay(250U) : mdelay(2U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_init(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 2U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_bufwr(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 3U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_bufrd(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 4U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_stall_set(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 5U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_stall_clear(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 6U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_toggle_set(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 7U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_toggle_clear(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 8U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_nullresp(void __iomem *base_address, unsigned char endpoint_channel, unsigned char null)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 9U, 1U, null);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_nackresp(void __iomem *base_address, unsigned char endpoint_channel, unsigned char nack)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 10U, 1U, nack);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_readyi_ready_inten(void __iomem *base_address, unsigned char endpoint_channel, unsigned char enable)
{
	unsigned long counter;

	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 12U, 1U, enable);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);

	return;
}

static inline void set_readyo_empty_inten(void __iomem *base_address, unsigned char endpoint_channel, unsigned char enable)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 13U, 1U, enable);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);

	return;
}

static inline void set_ping_inten(void __iomem *base_address, unsigned char endpoint_channel, unsigned char enable)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 14U, 1U, enable);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_stalled_inten(void __iomem *base_address, unsigned char endpoint_channel, unsigned char enable)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 15U, 1U, enable);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_nack_inten(void __iomem *base_address, unsigned char endpoint_channel, unsigned char enable)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 16U, 1U, enable);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void clear_readyi_ready_int_clr(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 18U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void clear_readyo_empty_int_clr(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 19U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void clear_ping_int_clr(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 20U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void clear_stalled_int_clr(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 21U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void clear_nack_int_clr(void __iomem *base_address, unsigned char endpoint_channel)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 22U, 1U, 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_et(void __iomem *base_address, unsigned char endpoint_channel, unsigned char type)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 23U, 2U, type);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

/* transfer type symbolic constant */
#define TYPE_UNUSED		0x0					/**< unused */
#define TYPE_CONTROL		0x0U					/**< control transfer */
#define TYPE_ISOCHRONOUS	0x1U					/**< isochronous transfer */
#define TYPE_BULK		0x2U					/**< bulk transfer */
#define TYPE_INTERRUPT		0x3U					/**< interrupt transfer */

static inline void set_dir(void __iomem *base_address, unsigned char endpoint_channel, unsigned char in)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 25U, 1U, in);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_bnum(void __iomem *base_address, unsigned char endpoint_channel, unsigned char buffers)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 26U, 2U, buffers - 1U);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_hiband(void __iomem *base_address, unsigned char endpoint_channel, unsigned char packets)
{
	unsigned long counter;
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 28U, 2U, packets);
	for (counter = 0xffffU; ((counter) && (get_register_bits(base_address, F_USB20HDC_REGISTER_EPCMD0 + endpoint_channel, 31U, 1U))); counter--);
	return;
}

static inline void set_reqspeed(void __iomem *base_address, unsigned char speed)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 0, 1U, speed);
	return;
}
/* bus speed request symbolic constant */
#define REQ_SPEED_HIGH_SPEED	0					/**< high-speed request */
#define REQ_SPEED_FULL_SPEED	1U					/**< full-speed request */

static inline void set_reqresume(void __iomem *base_address, unsigned char request)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 2U, 1U, request);
	return;
}

static inline void set_enrmtwkup(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 3U, 1U, enable);
	return;
}

static inline unsigned char get_enrmtwkup(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 3U, 1U);
}

static inline void set_disconnect(void __iomem *base_address, unsigned char disconnect)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 5U, 1U, disconnect);
	return;
}

static inline void set_physusp(void __iomem *base_address, unsigned char force_susped)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 16U, 1U, force_susped);
	return;
}

static inline void set_suspende_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 24U, 1U, enable);
	return;
}

static inline unsigned char get_suspende_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 24U, 1U);
}

static inline void set_suspendb_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 25U, 1U, enable);
	return;
}

static inline unsigned char get_suspendb_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 25U, 1U);
}

static inline void set_sof_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 26U, 1U, enable);
	return;
}

static inline unsigned char get_sof_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 26U, 1U);
}

static inline void set_setup_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 27U, 1U, enable);
	return;
}

static inline unsigned char get_setup_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 27U, 1U);
}

static inline void set_usbrste_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 28U, 1U, enable);
	return;
}

static inline unsigned char get_usbrste_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 28U, 1U);
}

static inline void set_usbrstb_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 29U, 1U, enable);
	return;
}

static inline unsigned char get_usbrstb_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 29U, 1U);
}

static inline void set_status_ok_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 30U, 1U, enable);
	return;
}

static inline unsigned char get_status_ok_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 30U, 1U);
}

static inline void set_status_ng_inten(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 31U, 1U, enable);
	return;
}

static inline unsigned char get_status_ng_inten(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVC, 31U, 1U);
}

static inline unsigned char get_suspend(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 0, 1U);
}

static inline unsigned char get_busreset(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 1U, 1U);
}

static inline unsigned char get_phyreset(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 16U, 1U);
}

static inline unsigned char get_crtspeed(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 17U, 1U);
}

/* bus speed symbolic constant */
#define CRT_SPEED_HIGH_SPEED		0				/**< high-speed */
#define CRT_SPEED_FULL_SPEED		1U				/**< full-speed */

static inline void clear_suspende_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 24U, 1U, 0);
	return;
}

static inline unsigned char get_suspende_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 24U, 1U);
}

static inline void clear_suspendb_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 25U, 1U, 0);
	return;
}

static inline unsigned char get_suspendb_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 25U, 1U);
}

static inline void clear_sof_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 26U, 1U, 0);
	return;
}

static inline unsigned char get_sof_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 26U, 1U);
}

static inline void clear_setup_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 27U, 1U, 0);
	return;
}

static inline unsigned char get_setup_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 27U, 1U);
}

static inline void clear_usbrste_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 28U, 1U, 0);
	return;
}

static inline unsigned char get_usbrste_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 28U, 1U);
}

static inline void clear_usbrstb_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 29U, 1U, 0);
	return;
}

static inline unsigned char get_usbrstb_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 29U, 1U);
}

static inline void clear_status_ok_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 30U, 1U, 0);
	return;
}

static inline unsigned char get_status_ok_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 30U, 1U);
}

static inline void clear_status_ng_int(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 31U, 1U, 0);
	return;
}

static inline unsigned char get_status_ng_int(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_DEVS, 31U, 1U);
}

static inline void set_func_addr(void __iomem *base_address, unsigned char address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_FADDR, 0, 7U, address);
	return;
}

static inline void set_dev_configured(void __iomem *base_address, unsigned char configured)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_FADDR, 8U, 1U, configured);
	return;
}

static inline unsigned char get_crt_func_addr(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_FADDR, 16U, 7U);
}

static inline unsigned short get_timstamp(void __iomem *base_address)
{
	return (unsigned short)get_register_bits(base_address, F_USB20HDC_REGISTER_TSTAMP, 0, 11U);
}

static inline void set_dm_pull_down(void __iomem *base_address, unsigned char pull_down)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGC, 7U, 1U, pull_down);
	return;
}

static inline void set_dp_pull_down(void __iomem *base_address, unsigned char pull_down)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGC, 8U, 1U, pull_down);
	return;
}

static inline void set_id_pull_up(void __iomem *base_address, unsigned char pull_up)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGC, 9U, 1U, pull_up);
	return;
}

static inline unsigned char get_otg_tmrout(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTS, 0, 1U);
}

static inline unsigned char get_id(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTS, 6U, 1U);
}

static inline unsigned char get_vbus_vld(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTS, 10U, 1U);
}

static inline unsigned char get_tmrout_c(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSC, 0, 1U);
}

static inline void clear_tmrout_c(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSC, 0, 1U, 0);
	return;
}

static inline unsigned char get_id_c(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSC, 6U, 1U);
}

static inline void clear_id_c(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSC, 6U, 1U, 0);
	return;
}

static inline unsigned char get_vbus_vld_c(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSC, 10U, 1U);
}

static inline void clear_vbus_vld_c(void __iomem *base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSC, 10U, 1U, 0);
	return;
}

static inline void set_id_fen(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSFALL, 6U, 1U, enable);
	return;
}

static inline unsigned char get_id_fen(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSFALL, 6U, 1U);
}

static inline void set_vbus_vld_fen(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSFALL, 10U, 1U, enable);
	return;
}

static inline unsigned char get_vbus_vld_fen(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSFALL, 10U, 1U);
}

static inline void set_otg_tmrout_ren(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSRISE, 0, 1U, enable);
	return;
}

static inline unsigned char get_otg_tmrout_ren(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSRISE, 0, 1U);
}

static inline void set_id_ren(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSRISE, 6U, 1U, enable);
	return;
}

static inline unsigned char get_id_ren(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSRISE, 6U, 1U);
}

static inline void set_vbus_vld_ren(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSRISE, 10U, 1U, enable);
	return;
}

static inline unsigned char get_vbus_vld_ren(void __iomem *base_address)
{
	return (unsigned char)get_register_bits(base_address, F_USB20HDC_REGISTER_OTGSTSRISE, 10U, 1U);
}

static inline void set_start_tmr(void __iomem *base_address, unsigned char start)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGTC, 0, 1U, start);
	return;
}

static inline void set_tmr_mode(void __iomem *base_address, unsigned char interval)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGTC, 1U, 1U, interval);
	return;
}

static inline void set_tmr_prescale(void __iomem *base_address, unsigned char prescale)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGTC, 8U, 8U, prescale);
	return;
}

static inline void set_tmr_init_val(void __iomem *base_address, unsigned short value)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_OTGT, 0, 16U, value);
	return;
}

static inline void set_dma_st(void __iomem *base_address, unsigned char dma_channel, unsigned char start)
{
	set_register_bits(base_address, dmac_register[dma_channel], 0, 1U, start);
	return;
}

static inline unsigned char get_dma_busy(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_register_bits(base_address, dmac_register[dma_channel], 1U, 1U);
}

static inline void set_dma_mode(void __iomem *base_address, unsigned char dma_channel, unsigned char mode)
{
	set_register_bits(base_address, dmac_register[dma_channel], 2U, 1U, mode);
	return;
}

/* DMA mode symbolic constant */
#define DMA_MDOE_DEMAND 		0				/**< demand transter */
#define DMA_MDOE_BLOCK			1U				/**< block transter */

static inline void set_dma_sendnull(void __iomem *base_address, unsigned char dma_channel, unsigned char send)
{
	set_register_bits(base_address, dmac_register[dma_channel], 3U, 1U, send);
	return;
}

static inline void set_dma_int_empty(void __iomem *base_address, unsigned char dma_channel, unsigned char empty)
{
	set_register_bits(base_address, dmac_register[dma_channel], 4U, 1U, empty);
	return;
}

static inline void set_dma_spr(void __iomem *base_address, unsigned char dma_channel, unsigned char spr)
{
	set_register_bits(base_address, dmac_register[dma_channel], 5U, 1U, spr);
	return;
}

static inline void set_dma_ep(void __iomem *base_address, unsigned char dma_channel, unsigned char endpoint_channel)
{
	set_register_bits(base_address, dmac_register[dma_channel], 8U, 4U, endpoint_channel);
	return;
}

static inline void set_dma_blksize(void __iomem *base_address, unsigned char dma_channel, unsigned short size)
{
	set_register_bits(base_address, dmac_register[dma_channel], 16U, 11U, size);
	return;
}

static inline unsigned char get_dma_np(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_register_bits(base_address, dmas_register[dma_channel], 0, 1U);
}

static inline unsigned char get_dma_sp(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_register_bits(base_address, dmas_register[dma_channel], 1U, 1U);
}

static inline void set_dma_tci(void __iomem *base_address, unsigned char dma_channel, unsigned long bytes)
{
	set_register_bits(base_address, dmatci_register[dma_channel], 0, 32U, bytes);
	return;
}

static inline unsigned long get_dma_tci(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned long)get_register_bits(base_address, dmatci_register[dma_channel], 0, 32U);
}

static inline unsigned long get_dma_tc(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned long)get_register_bits(base_address, dmatc_register[dma_channel], 0, 32U);
}

static inline void set_testp(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_TESTC, 0, 1U, enable);
	return;
}

static inline void set_testj(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_TESTC, 1U, 1U, enable);
	return;
}

static inline void set_testk(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_TESTC, 2U, 1U, enable);
	return;
}

static inline void set_testse0nack(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_TESTC, 3U, 1U, enable);
	return;
}

static inline void set_testforeceenable(void __iomem *base_address, unsigned char enable)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_TESTC, 4U, 1U, enable);
	return;
}

static inline void clear_epctrl(void __iomem *base_address, unsigned char endpoint_channel)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 0, 32U,
			  endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ? 0x00000500U : 0x00000400U);
	return;
}

static inline unsigned char get_ep_en(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 0, 1U);
}

static inline unsigned char get_et(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 1U, 2U);
}

static inline unsigned char get_dir(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 3U, 1U);
}

static inline unsigned char get_bnum(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 4U, 2U);
}

static inline unsigned char get_appptr(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 6U, 2U);
}

static inline unsigned char get_emptyi(void __iomem *base_address)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0, 8U, 1U);
}

static inline unsigned char get_fulli(void __iomem *base_address)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0, 9U, 1U);
}

static inline unsigned char get_phyptr(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 8U, 2U);
}

static inline unsigned char get_emptyo_empty(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 10U, 1U);
}

static inline unsigned char get_fullo_full(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 11U, 1U);
}

static inline unsigned char get_stall(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 12U, 1U);
}

static inline unsigned char get_toggle(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 13U, 1U);
}

static inline unsigned char get_hiband(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 14U, 2U);
}

static inline unsigned char get_nullresp(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 16U, 1U);
}

static inline unsigned char get_nackresp(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 17U, 1U);
}

static inline unsigned char get_readyi_ready_inten(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 18U, 1U);
}

static inline unsigned char get_readyo_empty_inten(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 19U, 1U);
}

static inline unsigned char get_ping_inten(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 20U ,1U);
}

static inline unsigned char get_stalled_inten(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 21U, 1U);
}

static inline unsigned char get_nack_inten(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 22U, 1U);
}

static inline unsigned char get_readyi_ready_int(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 26U, 1U);
}

static inline unsigned char get_readyo_empty_int(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 27U, 1U);
}

static inline unsigned char get_ping_int(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 28U, 1U);
}

static inline unsigned char get_stalled_int(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 29U, 1U);
}

static inline unsigned char get_nack_int(void __iomem *base_address, unsigned char endpoint_channel)
{
	return (unsigned char)get_epctrl_register_bits(base_address, F_USB20HDC_REGISTER_EPCTRL0 + endpoint_channel, 30U, 1U);
}

static inline void clear_epconf(void __iomem *base_address, unsigned char endpoint_channel)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCONF0 + endpoint_channel, 0, 32U, 0x00000000);
	return;
}

static inline void set_base(void __iomem *base_address, unsigned char endpoint_channel, unsigned short buffer_base_address)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCONF0 + endpoint_channel, 0, 13U, (buffer_base_address & 0x7fffU) >> 2);
	return;
}

static inline void set_size(void __iomem *base_address, unsigned char endpoint_channel, unsigned short size)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCONF0 + endpoint_channel, 13U, 11U, size);
	return;
}

static inline void set_countidx(void __iomem *base_address, unsigned char endpoint_channel, unsigned char index)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCONF0 + endpoint_channel, 24U, 5U, index);
	return;
}

static inline void clear_epcount(void __iomem *base_address, unsigned char index)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCOUNT0 + index, 0, 32U, 0x00000000);
	return;
}

static inline void set_appcnt(void __iomem *base_address, unsigned char index, unsigned short bytes)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCOUNT0 + index, 0, 11U, bytes);
	return;
}

static inline unsigned short get_appcnt(void __iomem *base_address, unsigned char index)
{
	return (unsigned short) get_register_bits(base_address, F_USB20HDC_REGISTER_EPCOUNT0 + index, 0, 11U);
}

static inline void set_phycnt(void __iomem *base_address, unsigned char index, unsigned short bytes)
{
	set_register_bits(base_address, F_USB20HDC_REGISTER_EPCOUNT0 + index, 16U, 11U, bytes);
	return;
}

static inline unsigned short get_phycnt(void __iomem *base_address, unsigned char index)
{
	return (unsigned short)get_register_bits(base_address, F_USB20HDC_REGISTER_EPCOUNT0 + index, 16U, 11U);
}

static inline unsigned long get_epbuf_address_offset(void)
{
	return f_usb20hdc_register[F_USB20HDC_REGISTER_EPBUF].address_offset - F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET;
}

static inline void write_epbuf(void __iomem *base_address, unsigned long offset, unsigned long *data, unsigned long bytes)
{
	memcpy(base_address + F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + offset, data, bytes);
	return;
}

static inline void read_epbuf(void __iomem *base_address, unsigned long offset, unsigned long *data, unsigned long bytes)
{
	memcpy(data, base_address + F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET + offset, bytes);
	return;
}

static inline unsigned long get_epbuf_dma_address(void __iomem *base_address, unsigned char dma_channel, unsigned long offset)
{
	return (unsigned long)(base_address + f_usb20hdc_register[F_USB20HDC_REGISTER_DMA1 + dma_channel].address_offset +
			       offset - (f_usb20hdc_register[F_USB20HDC_REGISTER_EPCTRL0].address_offset - F_USB20HDC_UDC_CONFIG_C_HSEL_AREA_ADDRESS_OFFSET));
}

/* So far, it is implementation of F_USB20HDC controller register                */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HDC DMA controller register                    */

/* F_USB20HDC HDMA controller register id enumeration constant */
#define F_USB20HDC_HDMAC_REGISTER_DMACR     0					/**< HDMAC overall configuration */
#define F_USB20HDC_HDMAC_REGISTER_DMACA     1U					/**< xch Configuration A */
#define F_USB20HDC_HDMAC_REGISTER_DMACB     2U					/**< xch Configuration B */
#define F_USB20HDC_HDMAC_REGISTER_DMACSA    3U					/**< xch Source Address */
#define F_USB20HDC_HDMAC_REGISTER_DMACDA    4U					/**< xch Destination Address */
#define F_USB20HDC_HDMAC_REGISTER_MAX	    5U					/**< Max Value */

/**
 * @brief F_USB20HDC HDMA controller register structures array
*/
static const struct {
	volatile unsigned long address_offset;					/**< register address offset */
	volatile unsigned char readable;					/**< register readable flag */
	volatile unsigned char writable;					/**< register writable flag */
} f_usb20hdc_hdmac_register[F_USB20HDC_HDMAC_REGISTER_MAX] = {
	{(volatile unsigned long)0x0000, 1U, 1U},				/* F_USB20HDC_HDMAC_REGISTER_DMACR */
	{(volatile unsigned long)0x0010U, 1U, 1U},				/* F_USB20HDC_HDMAC_REGISTER_DMACA */
	{(volatile unsigned long)0x0014U, 1U, 1U},				/* F_USB20HDC_HDMAC_REGISTER_DMACB */
	{(volatile unsigned long)0x0018U, 1U, 1U},				/* F_USB20HDC_HDMAC_REGISTER_DMACSA */
	{(volatile unsigned long)0x001cU, 1U, 1U},				/* F_USB20HDC_HDMAC_REGISTER_DMACDA */
};

static inline void set_hdmac_register_bits(void __iomem *base_address, unsigned char dma_channel, unsigned long register_id,
		unsigned char start_bit, unsigned char bit_length, unsigned long value)
{
	volatile unsigned long register_cache = 0x00;
	unsigned long mask = (unsigned long)-1 >> (32U - bit_length);

	value &= mask;

	if (f_usb20hdc_hdmac_register[register_id].readable)
		register_cache = __raw_readl(base_address + f_usb20hdc_hdmac_register[register_id].address_offset + dma_channel * 0x10U);

	register_cache &= ~(mask << start_bit);
	register_cache |= (value << start_bit);

	if (f_usb20hdc_hdmac_register[register_id].writable)
		__raw_writel(register_cache, base_address + f_usb20hdc_hdmac_register[register_id].address_offset + dma_channel * 0x10U);

	return;
}

static inline unsigned long get_hdmac_register_bits(void __iomem *base_address, unsigned char dma_channel, unsigned long register_id,
		unsigned char start_bit, unsigned char bit_length)
{
	volatile unsigned long register_cache = 0x00;
	unsigned short mask = (unsigned long)-1 >> (32U - bit_length);

	if (f_usb20hdc_hdmac_register[register_id].readable)
		register_cache = __raw_readl(base_address + f_usb20hdc_hdmac_register[register_id].address_offset + dma_channel * 0x10U);

	return register_cache >> start_bit & mask;
}

static inline void set_hdmac_dmacr_register_bits(void __iomem *base_address, unsigned char start_bit, unsigned char bit_length, unsigned long value)
{
	volatile unsigned long register_cache = 0x00;
	unsigned long mask = (unsigned long)-1 >> (32U - bit_length);

	value &= mask;

	if (f_usb20hdc_hdmac_register[F_USB20HDC_HDMAC_REGISTER_DMACR].readable)
		register_cache = __raw_readl(base_address + f_usb20hdc_hdmac_register[F_USB20HDC_HDMAC_REGISTER_DMACR].address_offset);

	register_cache &= ~(mask << start_bit);
	register_cache |= (value << start_bit);

	if (f_usb20hdc_hdmac_register[F_USB20HDC_HDMAC_REGISTER_DMACR].writable)
		__raw_writel(register_cache, base_address + f_usb20hdc_hdmac_register[F_USB20HDC_HDMAC_REGISTER_DMACR].address_offset);

	return;
}

static inline unsigned long get_hdmac_dmacr_register_bits(void __iomem *base_address, unsigned char start_bit, unsigned char bit_length)
{
	volatile unsigned long register_cache = 0x00;
	unsigned short mask = (unsigned long)-1 >> (32U - bit_length);

	if (f_usb20hdc_hdmac_register[F_USB20HDC_HDMAC_REGISTER_DMACR].readable)
		register_cache = __raw_readl(base_address + f_usb20hdc_hdmac_register[F_USB20HDC_HDMAC_REGISTER_DMACR].address_offset);

	return register_cache >> start_bit & mask;
}

static inline void set_dh(void __iomem *base_address, unsigned char halt)
{
	set_hdmac_dmacr_register_bits(base_address, 24U, 4U, halt);
	return;
}

static inline void set_pr(void __iomem *base_address, unsigned char rotation)
{
	set_hdmac_dmacr_register_bits(base_address, 28U, 1U, rotation);
	return;
}

static inline unsigned char get_ds(void __iomem *base_address)
{
	return (unsigned char)get_hdmac_dmacr_register_bits(base_address, 30U, 1U);
}

static inline void set_de(void __iomem *base_address, unsigned char enable)
{
	set_hdmac_dmacr_register_bits(base_address, 31U, 1U, enable);
	return;
}

static inline void set_tc(void __iomem *base_address, unsigned char dma_channel, unsigned long count)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACA, 0, 16U, count - 1U);
	return;
}

static inline void set_bc(void __iomem *base_address, unsigned char dma_channel, unsigned char count)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACA, 16U, 4U, count - 1U);
	return;
}

static inline void set_bt(void __iomem *base_address, unsigned char dma_channel, unsigned char type)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACA, 20U, 4U, type);
	return;
}

/* transfer beat type symbolic constant */
#define BT_NORMAL			0x0				/**< NORMAL */
#define BT_SINGLE			0x8U				/**< SINGLE */
#define BT_INCR 			0x9U				/**< INCR */
#define BT_WRAP4			0xaU				/**< WRAP4 */
#define BT_INCR4			0xbU				/**< INCR4 */
#define BT_WRAP8			0xcU				/**< WRAP8 */
#define BT_INCR8			0xdU				/**< INCR8 */
#define BT_WRAP16			0xeU				/**< WRAP16 */
#define BT_INCR16			0xfU				/**< INCR16 */

static inline void set_is(void __iomem *base_address, unsigned char dma_channel, unsigned char select)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACA, 24U, 5U, select);
	return;
}

/* input select symbolic constant */
#define IS_SOFTWEAR			0x00				/**< Softwear */
#define IS_DREQ_HIGH			0x0eU				/**< DREQ High level or Positive edge */
#define IS_DREQ_LOW			0x0fU				/**< DREQ Low level or Negative edge */
#define IS_IDREQ0			0x10U				/**< IDREQ[0] High level or Positive edge */
#define IS_IDREQ1			0x11U				/**< IDREQ[1] High level or Positive edge */
#define IS_IDREQ2			0x12U				/**< IDREQ[2] High level or Positive edge */
#define IS_IDREQ3			0x13U				/**< IDREQ[3] High level or Positive edge */
#define IS_IDREQ4			0x14U				/**< IDREQ[4] High level or Positive edge */
#define IS_IDREQ5			0x15U				/**< IDREQ[5] High level or Positive edge */
#define IS_IDREQ6			0x16U				/**< IDREQ[6] High level or Positive edge */
#define IS_IDREQ7			0x17U				/**< IDREQ[7] High level or Positive edge */
#define IS_IDREQ8			0x18U				/**< IDREQ[8] High level or Positive edge */
#define IS_IDREQ9			0x19U				/**< IDREQ[9] High level or Positive edge */
#define IS_IDREQ10			0x1aU				/**< IDREQ[10] High level or Positive edge */
#define IS_IDREQ11			0x1bU				/**< IDREQ[11] High level or Positive edge */
#define IS_IDREQ12			0x1cU				/**< IDREQ[12] High level or Positive edge */
#define IS_IDREQ13			0x1dU				/**< IDREQ[13] High level or Positive edge */
#define IS_IDREQ14			0x1eU				/**< IDREQ[14] High level or Positive edge */
#define IS_IDREQ15			0x1fU				/**< IDREQ[15] High level or Positive edge */

static inline void set_st(void __iomem *base_address, unsigned char dma_channel, unsigned char use)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACA, 29U, 1U, use);
	return;
}

static inline void set_pb(void __iomem *base_address, unsigned char dma_channel, unsigned char pause)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACA, 30U, 1U, pause);
	return;
}

static inline void set_eb(void __iomem *base_address, unsigned char dma_channel, unsigned char enable)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACA, 31U, 1U, enable);
	return;
}

static inline unsigned char get_eb(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACA, 31U, 1U);
}

static inline void set_dp(void __iomem *base_address, unsigned char dma_channel, unsigned char protection)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 8U, 4U, protection);
	return;
}

static inline void set_sp(void __iomem *base_address, unsigned char dma_channel, unsigned char protection)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 12U, 4U, protection);
	return;
}

static inline void clear_ss(void __iomem *base_address, unsigned char dma_channel)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 16U, 3U, 0);
	return;
}

static inline unsigned char get_ss(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 16U, 3U);
}

/* stop status symbolic constant */
#define SS_INITIAL_VALUE		0x0				/**< Initial value */
#define SS_ADDRESS_OVERFLOW		0x1U				/**< Address overflow */
#define SS_TRANSFER_STOP_REQUEST	0x2U				/**< Transfer stop request */
#define SS_SOURCE_ACCESS_ERROR		0x3U				/**< Source access error */
#define SS_DESTINATION_ACCESS_ERROR	0x4U				/**< Destination access error */
#define SS_NORMAL_END			0x5U				/**< normal end */
#define SS_DMA_PAUSE			0x7U				/**< DMA pause */

static inline void set_ci(void __iomem *base_address, unsigned char dma_channel, unsigned char enable)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 19U, 1U, enable);
	return;
}

static inline unsigned char get_ci(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 19U, 1U);
}

static inline void set_ei(void __iomem *base_address, unsigned char dma_channel, unsigned char enable)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 20U, 1U, enable);
	return;
}

static inline unsigned char get_ei(void __iomem *base_address, unsigned char dma_channel)
{
	return (unsigned char)get_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 20U, 1U);
}

static inline void set_rd(void __iomem *base_address, unsigned char dma_channel, unsigned char reload)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 21U, 1U, reload);
	return;
}

static inline void set_rs(void __iomem *base_address, unsigned char dma_channel, unsigned char reload)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 22U, 1U, reload);
	return;
}

static inline void set_rc(void __iomem *base_address, unsigned char dma_channel, unsigned char reload)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 23U, 1U, reload);
	return;
}

static inline void set_fd(void __iomem *base_address, unsigned char dma_channel, unsigned char fixed)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 24U, 1U, fixed);
	return;
}

static inline void set_fs(void __iomem *base_address, unsigned char dma_channel, unsigned char fixed)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 25U, 1U, fixed);
	return;
}

static inline void set_tw(void __iomem *base_address, unsigned char dma_channel, unsigned char width)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 26U, 2U, width);
	return;
}

/* transfer bus width symbolic constant */
#define TW_BYTE 			0x0				/**< Byte */
#define TW_HWORD			0x1U				/**< Half-word */
#define TW_WORD 			0x2U				/**< Word */

static inline void set_ms(void __iomem *base_address, unsigned char dma_channel, unsigned char mode)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 28U, 2U, mode);
	return;
}

/* transfer mode symbolic constant */
#define MS_BLOCK			0x0				/**< Block transfer mode */
#define MS_BURST			0x1U				/**< Burst transfer mode */
#define MS_DEMAND			0x2U				/**< Demand transfer mode */

static inline void set_tt(void __iomem *base_address, unsigned char dma_channel, unsigned char type)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACB, 30U, 2U, type);
	return;
}
/* transfer type symbolic constant */
#define TT_2CYCLE_TRANSFER		0x0				/**< 2cycle transfer */

static inline void set_dmacsa(void __iomem *base_address, unsigned char dma_channel, unsigned long address)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACSA, 0, 32U, address);
	return;
}

static inline void set_dmacda(void __iomem *base_address, unsigned char dma_channel, unsigned long address)
{
	set_hdmac_register_bits(base_address, dma_channel, F_USB20HDC_HDMAC_REGISTER_DMACDA, 0, 32U, address);
	return;
}

/* So far, it is implementation of F_USB20HDC DMA controller register                */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HDC UDC driver configuration function            */

static inline void __iomem *remap_iomem_region(unsigned long physical_base_address, unsigned long size)
{
	/* convert physical address to virtula address */
	return (void __iomem *)IO_ADDRESS(physical_base_address);
}

static inline void unmap_iomem_region(void __iomem *virtual_base_address)
{
	return;
}

static inline unsigned long get_irq_flag(void)
{
	return IRQF_SHARED;
}

static inline unsigned long get_dma_irq_flag(unsigned char dma_channel)
{
	return IRQF_SHARED;
}

static inline void enable_controller(void __iomem *virtual_base_address, unsigned char enable)
{
	return;
}

static inline void enable_dma_controller(void __iomem *virtual_base_address, unsigned char enable)
{
	return;
}

static inline void enable_bus_connect_detect_controller(void __iomem *virtual_base_address, unsigned char enable)
{
	return;
}

static inline void enable_dplus_line_pullup_controller(void __iomem *virtual_base_address, unsigned char enable)
{
	return;
}

static inline void set_bus_connect_detect(void __iomem *virtual_base_address, unsigned char connect)
{
	return;
}

static inline unsigned char is_bus_connected(void __iomem *virtual_base_address)
{
	/* get bus connected */
	return fusb_otg_get_vbus() ? 1U : 0;
}

static inline void enable_dplus_line_pullup(void __iomem *virtual_base_address, unsigned char enable)
{
	if (enable) {
		/* pull-up D+ terminal */
		set_physusp(virtual_base_address, 0);
		set_disconnect(virtual_base_address, 0);
	} else {
		/* pull-down D+ terminal */
		set_disconnect(virtual_base_address, 1U);
		set_physusp(virtual_base_address, 1U);
	}
	return;
}

#define f_usb_spin_lock_irqsave(hdc, flags)		\
do {							\
	spin_lock_irqsave(&hdc->lock, flags);		\
	hdc->lock_cnt++;				\
} while (0)
#define f_usb_spin_unlock_irqrestore(hdc, flags)	\
do {							\
	hdc->lock_cnt--;				\
	spin_unlock_irqrestore(&hdc->lock, flags);	\
} while (0)
#define f_usb_spin_lock(hdc)				\
do {							\
	spin_lock(&hdc->lock);				\
	hdc->lock_cnt++;				\
} while (0)
#define f_usb_spin_unlock(hdc)				\
do {							\
	hdc->lock_cnt--;				\
	spin_unlock(&hdc->lock);			\
} while (0)

/** 32 byte alignment macro */
#define f_usb_roundup_32(len)	(((unsigned long)len + 31UL) & ~31UL)

static inline dma_addr_t f_usb_dma_map_single(struct device *dev, void *cpu_addr,
		size_t size, enum dma_data_direction dir)
{
	return dma_map_single(dev, cpu_addr, (size_t)f_usb_roundup_32(size), dir);
}

static inline void f_usb_dma_unmap_single(struct device *dev, dma_addr_t handle,
		size_t size, enum dma_data_direction dir)
{
	dma_unmap_single(dev, handle, (size_t)f_usb_roundup_32(size), dir);
}

static inline void f_usb_dma_sync_single_for_cpu(struct device *dev,
		dma_addr_t handle, size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_cpu(dev, handle, (size_t)f_usb_roundup_32(size), dir);
}

static inline void f_usb_dma_sync_single_for_device(struct device *dev,
		dma_addr_t handle, size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_device(dev, handle, (size_t)f_usb_roundup_32(size), dir);
}

/* So far, it is implementation of F_USB20HDC UDC driver configuration function            */
/************************************************************************************************/

#endif
