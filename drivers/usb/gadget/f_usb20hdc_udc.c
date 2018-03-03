/*
 * drivers/usb/gadget/f_usb20hdc_udc.c
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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <asm/dma-mapping.h>
#include "linux/usb/f_usb/f_usb20hdc_ioctl.h"
#include "linux/usb/f_usb/usb_otg_control.h"
#include "f_usb20hdc_udc.h"

//#define FUSB_FUNC_TRACE

#ifdef FUSB_FUNC_TRACE
#define fusb_func_trace(b)		\
	printk("CALL %s %s(%d)\n",(b),__FUNCTION__,__LINE__)
#define hdc_dbg(dev, format, arg...)	\
	dev_dbg(dev, format, ## arg)
#define hdc_err(dev, format, arg...)	\
	dev_err(dev, format, ## arg)
#else
#define fusb_func_trace(b)
#define hdc_dbg(dev, format, arg...)
#define hdc_err(dev, format, arg...)
#endif

struct f_usb20hdc_udc_request {
	struct usb_request	request;			/**< USB request structure */
	struct list_head	queue;				/**< request queue head */
	unsigned char		request_execute;		/**< request execute flag */
	unsigned char		dma_transfer_buffer_map;	/**< DMA transfer buffer map flag */
};

struct f_usb20hdc_udc_endpoint {
	struct usb_ep				endpoint;			/**< endpoint structure */
	struct f_usb20hdc_udc			*f_usb20hdc;			/**< F_USB20HDC UDC device driver structure */
	unsigned char				endpoint_channel;		/**< endpoint channel */
	unsigned char				transfer_direction;		/**< endpoint transfer direction flag */
	unsigned char				transfer_type;			/**< endpoint transfer type */
	unsigned short				buffer_address_offset[F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT];
	/**< endpoint buffer address offset */
	unsigned short				buffer_size;			/**< endpoint buffer size */
	unsigned char				buffers;			/**< endpoint buffer count */
	unsigned char				pio_transfer_auto_change;	/**< PIO transfer auto change flag */
	unsigned char				in_transfer_end_timinig_host_transfer;
	/**< IN transfer end notify timing to USB host flag */
	signed char				usb_dma_channel;		/**< USB controller DMA channel */
	signed char				dma_channel;			/**< DMA controller DMA channel */
	unsigned char				dma_request_channel;		/**< DMA controller DMA request channel */
	const struct usb_endpoint_descriptor	*endpoint_descriptor;
	/**< endpoint descriptor structure */
	struct f_usb20hdc_udc_request		*request;			/**< current request structure */
	struct list_head			queue;				/**< endpoint queue head */
	unsigned char				halt;				/**< transfer halt flag */
	unsigned char				force_halt; 			/**< transfer force halt flag */
	unsigned char				null_packet_transfer;		/**< NULL packet transfer flag */
	unsigned char				dma_transfer;			/**< DMA transfer flag */
	void __iomem				*dma_transfer_buffer_address;	/**< DMA transfer buffer pyhsical address */
};

struct f_usb20hdc_udc {
	struct usb_gadget		gadget;					/**< gadget structure */
	struct usb_gadget_driver	*gadget_driver;				/**< gadget driver structure */
	spinlock_t			lock;					/**< mutex */
	int				lock_cnt;				/**< mutex Lock Count */
	struct timer_list		timer;					/**< timer structure for bus connect check */
	struct resource 		*resource;				/**< F_USB20HDC device resource */
	void __iomem			*register_base_address;			/**< F_USB20HDC register base address */
	int				irq;					/**< F_USB20HDC IRQ number */
	/**< F_USB20HDC DMAC device resource */
	void __iomem			*dmac_register_base_address[F_USB20HDC_UDC_CONFIG_DMA_CHANNELS];
	/**< F_USB20HDC DMAC register base address */
	int				dma_irq[F_USB20HDC_UDC_CONFIG_DMA_CHANNELS];
	/**< F_USB20HDC DMA IRQ number */
	struct f_usb20hdc_udc_endpoint	endpoint[F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS];
	/**< F_USB20HDC UDC device driver endpoint structure array */
	unsigned char			device_driver_register;			/**< device driver register flag */
	unsigned char			highspeed_support;			/**< high-speed support flag */
	unsigned char			bus_connect;				/**< bus connect status flag */
	unsigned char			selfpowered;				/**< self-powered flag */
	enum usb_device_state		device_state;				/**< USB device state */
	enum usb_device_state		device_state_last;			/**< last USB device state */
	unsigned char			configure_value_last;			/**< last configure value */
	unsigned char			control_transfer_stage;			/**< control transfer stage */
	#define CONTROL_TRANSFER_STAGE_SETUP		0			/**< SETUP stage */
	#define CONTROL_TRANSFER_STAGE_IN_DATA		1U			/**< IN data stage */
	#define CONTROL_TRANSFER_STAGE_OUT_DATA		2U			/**< OUT data stage */
	#define CONTROL_TRANSFER_STAGE_IN_STATUS	3U			/**< IN status stage */
	#define CONTROL_TRANSFER_STAGE_OUT_STATUS	4U			/**< OUT status stage */
	#define CONTROL_TRANSFER_STAGE_MAX		5U			/**< max value */
	unsigned char			control_transfer_priority_direction;	/**< control transfer priority-processing direction flag */
	unsigned char			control_transfer_status_stage_delay;	/**< control transfer status stage delay flag */
	unsigned char			usb_dma_endpoint_channel[F_USB20HDC_UDC_CONFIG_USB_DMA_CHANNELS];
	/**< USB DMA endpoint channel array */
	unsigned char			dma_endpoint_channel[F_USB20HDC_UDC_CONFIG_DMA_CHANNELS];
	/**< DMA endpoint channel array */
};

/**
 * @brief f_usb20hdc_data
 *
 * F_USB20HDC UDC device driver structure data for usb_gadget_register_driver() & usb_gadget_unregister_driver()
 */
static struct f_usb20hdc_udc *f_usb20hdc_data;

/**
 * @brief f_usb20hdc_pullup_flag
 *
 *
 */
static int f_usb20hdc_pullup_flag = 0;

/**
 * @brief hcd_otg_ops
 *
 *
 */
struct usb_otg_module_gadget_ops hcd_otg_ops;

/************************************************************************************************/
/* Implementation of here to DMAC control local function for F_USB20HDC UDC device driver    */

static void f_usb20hdc_initialize_dma_controller(struct f_usb20hdc_udc *f_usb20hdc)
{
	unsigned long counter;
	signed char dma_channel;

	/* initialize F_USB20HDC DMA controller register */
	for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		dma_channel = f_usb20hdc->endpoint[counter].dma_channel;
		if (dma_channel == -1)
			continue;
		set_tc(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 1U);
		set_bc(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 1U);
		set_bt(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, BT_NORMAL);
		set_is(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, IS_SOFTWEAR);
		set_st(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_pb(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_eb(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_dp(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_sp(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		clear_ss(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel);
		set_ci(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_ei(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_rd(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_rs(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_rc(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_fd(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_fs(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_tw(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, TW_BYTE);
		set_ms(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, MS_BLOCK);
		set_tt(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, TT_2CYCLE_TRANSFER);
		set_dmacsa(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		set_dmacda(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
	}

	return;
}

static void f_usb20hdc_set_dma_controller(struct f_usb20hdc_udc_endpoint *endpoint, unsigned long source, unsigned long destination, unsigned long bytes, unsigned char last_transfer)
{
	static const unsigned char dma_request_register_converter[] = {
		IS_SOFTWEAR,	/* none */
		IS_DREQ_HIGH,	/* DREQ High level or Positive edge */
		IS_IDREQ0,	/* IDREQ[0] High level or Positive edge */
		IS_IDREQ1,	/* IDREQ[1] High level or Positive edge */
		IS_IDREQ2,	/* IDREQ[2] High level or Positive edge */
		IS_IDREQ3,	/* IDREQ[3] High level or Positive edge */
		IS_IDREQ4,	/* IDREQ[4] High level or Positive edge */
		IS_IDREQ5,	/* IDREQ[5] High level or Positive edge */
		IS_IDREQ6,	/* IDREQ[6] High level or Positive edge */
		IS_IDREQ7,	/* IDREQ[7] High level or Positive edge */
		IS_IDREQ8,	/* IDREQ[8] High level or Positive edge */
		IS_IDREQ9,	/* IDREQ[9] High level or Positive edge */
		IS_IDREQ10,	/* IDREQ[10] High level or Positive edge */
		IS_IDREQ11,	/* IDREQ[11] High level or Positive edge */
		IS_IDREQ12,	/* IDREQ[12] High level or Positive edge */
		IS_IDREQ13,	/* IDREQ[13] High level or Positive edge */
		IS_IDREQ14,	/* IDREQ[14] High level or Positive edge */
		IS_IDREQ15,	/* IDREQ[15] High level or Positive edge */
	};
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	unsigned char endpoint_channel = endpoint->endpoint_channel;
	signed char usb_dma_channel = endpoint->usb_dma_channel;
	signed char dma_channel = endpoint->dma_channel;

	/* check DMA channel */
	if (unlikely((usb_dma_channel == -1) || (dma_channel == -1)))
		return;

	/* setup F_USB20HDC DMA controller register */
	f_usb20hdc->dma_endpoint_channel[dma_channel] = endpoint_channel;
	set_is(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, dma_request_register_converter[endpoint->dma_request_channel]);
	set_bc(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 1U);
	set_tt(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, TT_2CYCLE_TRANSFER);
	set_tw(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, TW_WORD);
	set_rc(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
	set_dmacsa(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, source);
	set_rs(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
	set_fs(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, endpoint->transfer_direction ? 0: 1);
	set_dmacda(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, destination);
	set_rd(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
	set_fd(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, endpoint->transfer_direction ? 1: 0);
	if (!(bytes % 64U)) {
		set_ms(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, MS_BLOCK);
		set_bt(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, BT_INCR16);
		set_tc(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, bytes / 64U);
		set_dma_blksize(f_usb20hdc->register_base_address, usb_dma_channel, 64U);
	} else {
		set_ms(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, MS_DEMAND);
		set_bt(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, BT_SINGLE);
		set_tc(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, bytes % 4 ? bytes / 4 + 1 : bytes / 4);
		set_dma_blksize(f_usb20hdc->register_base_address, usb_dma_channel, 4);
	}

	/* setup F_USB20HDC controller register */
	f_usb20hdc->usb_dma_endpoint_channel[usb_dma_channel] = endpoint_channel;
	set_dma_mode(f_usb20hdc->register_base_address, usb_dma_channel, DMA_MDOE_DEMAND);
	if (endpoint->transfer_direction) {
		if (last_transfer) {
			set_dma_sendnull(f_usb20hdc->register_base_address, usb_dma_channel, endpoint->null_packet_transfer ? 1U : 0);
			set_dma_int_empty(f_usb20hdc->register_base_address, usb_dma_channel, endpoint->in_transfer_end_timinig_host_transfer ? 1U : 0);
		} else {
			set_dma_sendnull(f_usb20hdc->register_base_address, usb_dma_channel, 0);
			set_dma_int_empty(f_usb20hdc->register_base_address, usb_dma_channel, 0);
		}
	} else {
		set_dma_spr(f_usb20hdc->register_base_address, usb_dma_channel, 0);
	}
	set_dma_ep(f_usb20hdc->register_base_address, usb_dma_channel, endpoint_channel);
	set_dma_tci(f_usb20hdc->register_base_address, usb_dma_channel, bytes);

	return;
}

static void f_usb20hdc_enable_dma_transfer(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char enable)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	signed char usb_dma_channel = endpoint->usb_dma_channel;
	signed char dma_channel = endpoint->dma_channel;

	/* check DMA channel */
	if (unlikely((usb_dma_channel == -1) || (dma_channel == -1)))
		return;

	if (enable) {
		/* enable DMA transfer */
		set_ei(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 1U);
		set_dma_inten(f_usb20hdc->register_base_address, usb_dma_channel, 1U);
		set_de(f_usb20hdc->dmac_register_base_address[dma_channel], 1U);
		set_eb(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 1U);
		set_dma_st(f_usb20hdc->register_base_address, usb_dma_channel, 1U);
	} else {
		/* disable DMA transfer */
		set_dma_st(f_usb20hdc->register_base_address, usb_dma_channel, 0);
		set_dma_inten(f_usb20hdc->register_base_address, usb_dma_channel, 0);
		set_ei(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
		if (get_eb(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel)) {
			set_eb(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel, 0);
			for (; get_eb(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel););
		}
		clear_ss(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel);
		clear_dma_int(f_usb20hdc->register_base_address, usb_dma_channel);
		endpoint->dma_transfer = 0;
	}

	return;
}

static unsigned char f_usb20hdc_set_in_transfer_dma(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel = endpoint->endpoint_channel;
	unsigned long bytes;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!endpoint->endpoint_descriptor)) {
		f_usb20hdc_enable_dma_transfer(endpoint, 0);
		return 0;
	}

	/* check NULL packet IN transfer for last packet of transaction */
	if ((request->request.length) && (!(request->request.length % endpoint->endpoint.maxpacket)) && (request->request.zero))
		endpoint->null_packet_transfer = 1U;

	/* check DMA transfer buffer mapping */
	if ((request->request.dma == ~(dma_addr_t)0) || (request->request.dma == 0)) {
		/* map DMA transfer buffer and synchronize DMA transfer buffer */
		request->request.dma = f_usb_dma_map_single(f_usb20hdc->gadget.dev.parent, request->request.buf, request->request.length, DMA_TO_DEVICE);
		request->dma_transfer_buffer_map = 1U;
	} else {
		/* synchronize DMA transfer buffer */
		f_usb_dma_sync_single_for_device(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_TO_DEVICE);
		request->dma_transfer_buffer_map = 0;
	}

	/* check DMA transfer address align */
	if (request->request.dma & 0x3) {
		dev_dbg(f_usb20hdc->gadget.dev.parent, "%s():DMA controller can not setup at dma address = 0x%p.\n",
			__FUNCTION__, (void *)request->request.dma);
		f_usb20hdc_enable_dma_transfer(endpoint, 0);
		if (request->dma_transfer_buffer_map) {
			f_usb_dma_unmap_single(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_TO_DEVICE);
			request->request.dma = ~(dma_addr_t)0;
			request->dma_transfer_buffer_map = 0;
		}

		fusb_func_trace("END 1");
		return 0;
	}

	/* calculate this time transfer byte */
	bytes = request->request.length < F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES ?
		request->request.length : F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES;
	if (bytes / endpoint->endpoint.maxpacket)
		bytes -= bytes % endpoint->endpoint.maxpacket ;
	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u DMA is setup at length = %u, actual = %u, max packet = %u, this time = %u.\n",
		endpoint_channel, request->request.length, request->request.actual, endpoint->endpoint.maxpacket, (unsigned int)bytes);

	/* update actual byte */
	request->request.actual = bytes;

	/* set request execute */
	request->request_execute = 1U;

	/* setup DMA transfer */
	f_usb20hdc_set_dma_controller(endpoint, (unsigned long)request->request.dma, (unsigned long)endpoint->dma_transfer_buffer_address,
				      bytes, !(request->request.length - request->request.actual) ? 1U : 0);

	/* enable DMA transfer */
	f_usb20hdc_enable_dma_transfer(endpoint, 1U);

	fusb_func_trace("END 2");
	return 1U;
}

static unsigned char f_usb20hdc_set_out_transfer_dma(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel = endpoint->endpoint_channel;
	unsigned long bytes;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!endpoint->endpoint_descriptor))
		return 0;

	/* check DMA transfer buffer mapping */
	if ((request->request.dma == ~(dma_addr_t)0) || (request->request.dma == 0)) {
		/* map DMA transfer buffer and synchronize DMA transfer buffer */
		request->request.dma = f_usb_dma_map_single(f_usb20hdc->gadget.dev.parent, request->request.buf, request->request.length, DMA_FROM_DEVICE);
		request->dma_transfer_buffer_map = 1U;
	} else {
		/* synchronize DMA transfer buffer */
		f_usb_dma_sync_single_for_device(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_FROM_DEVICE);
		request->dma_transfer_buffer_map = 0;
	}

	/* check DMA transfer address align and DMA transfer length */

	if (request->request.dma & 0x3) {
		dev_dbg(f_usb20hdc->gadget.dev.parent, "%s():DMA controller can not setup at dma address = 0x%p, dma length = %u.\n",
			__FUNCTION__, (void *)request->request.dma, request->request.length);
		f_usb20hdc_enable_dma_transfer(endpoint, 0);
		if (request->dma_transfer_buffer_map) {
			f_usb_dma_unmap_single(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_FROM_DEVICE);
			request->request.dma = ~(dma_addr_t)0;
			request->dma_transfer_buffer_map = 0;
		}

		fusb_func_trace("END 1");
		return 0;
	}

	/* calculate this time transfer byte */
	bytes = request->request.length < F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES ?
		request->request.length : F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES;
	if (bytes / endpoint->endpoint.maxpacket)
		bytes -= bytes % endpoint->endpoint.maxpacket ;
	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u DMA is setup at length = %u, actual = %u, max packet = %u, this time = %u.\n",
		endpoint_channel, request->request.length, request->request.actual, endpoint->endpoint.maxpacket, (unsigned int)bytes);

	/* set request execute */
	request->request_execute = 1U;

	/* setup DMA transfer */
	f_usb20hdc_set_dma_controller(endpoint, (unsigned long)endpoint->dma_transfer_buffer_address, (unsigned long)request->request.dma, bytes, 0);

	/* enable DMA transfer */
	f_usb20hdc_enable_dma_transfer(endpoint, 1U);

	fusb_func_trace("END 2");
	return 1U;
}

static void f_usb20hdc_initialize_endpoint(struct f_usb20hdc_udc_endpoint *, unsigned char, unsigned char, unsigned char);      /**< prototype */

static void f_usb20hdc_abort_in_transfer_dma(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char initalize)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char dma_transfer = endpoint->dma_transfer;

	fusb_func_trace("START");

	/* disable DMA transfer */
	f_usb20hdc_enable_dma_transfer(endpoint, 0);

	/* check DMA transfer buffer unmap */
	if ((dma_transfer) && (request->request.dma != ~(dma_addr_t)0)) {
		if (request->dma_transfer_buffer_map) {
			/* unmap DMA transfer buffer and synchronize DMA transfer buffer */
			f_usb_dma_unmap_single(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_TO_DEVICE);
			f_usb_dma_sync_single_for_cpu(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_TO_DEVICE);
			request->request.dma = ~(dma_addr_t)0;
			request->dma_transfer_buffer_map = 0;
		} else {
			/* synchronize DMA transfer buffer */
			f_usb_dma_sync_single_for_cpu(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_TO_DEVICE);
		}
	}

	/* clear NULL packet transfer */
	endpoint->null_packet_transfer = 0;

	if (!initalize)
		return;

	/* initialize endpoint */
	f_usb20hdc_initialize_endpoint(endpoint, 1U, 0, 0);

	fusb_func_trace("END");

	return;
}

static void f_usb20hdc_abort_out_transfer_dma(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char initalize)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char dma_transfer = endpoint->dma_transfer;

	fusb_func_trace("START");

	/* disable DMA transfer */
	f_usb20hdc_enable_dma_transfer(endpoint, 0);

	/* check DMA transfer buffer unmap */
	if ((dma_transfer) && (request->request.dma != ~(dma_addr_t)0)) {
		if (request->dma_transfer_buffer_map) {
			/* unmap DMA transfer buffer and synchronize DMA transfer buffer */
			f_usb_dma_unmap_single(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_FROM_DEVICE);
			f_usb_dma_sync_single_for_cpu(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_FROM_DEVICE);
			request->request.dma = ~(dma_addr_t)0;
			request->dma_transfer_buffer_map = 0;
		} else {
			/* synchronize DMA transfer buffer */
			f_usb_dma_sync_single_for_cpu(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length, DMA_FROM_DEVICE);
		}
	}

	if (!initalize)
		return;

	/* initialize endpoint */
	f_usb20hdc_initialize_endpoint(endpoint, 1U, 0, 0);

	fusb_func_trace("END");
	return;
}

static unsigned char f_usb20hdc_end_in_transfer_dma(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel;
	unsigned long bytes;
	unsigned long index;

	fusb_func_trace("START");

	f_usb20hdc = endpoint->f_usb20hdc;
	endpoint_channel = endpoint->endpoint_channel;

	/* check transfer remain byte */
	if (!(request->request.length - request->request.actual)) {
		/* complete request */
		f_usb20hdc_abort_in_transfer_dma(endpoint, 0);
		fusb_func_trace("END 1");
		return 1U;
	}

	/* calculate this time transfer byte */
	bytes = (request->request.length - request->request.actual) < F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES ?
		request->request.length - request->request.actual : F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES;
	if (bytes / endpoint->endpoint.maxpacket)
		bytes -= bytes % endpoint->endpoint.maxpacket ;
	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u DMA is setup at length = %u, actual = %u, max packet = %u, this time = %u.\n",
		endpoint_channel, request->request.length, request->request.actual, endpoint->endpoint.maxpacket, (unsigned int)bytes);

	/* update actual byte */
	index = request->request.actual;
	request->request.actual += bytes;

	/* setup DMA transfer */
	f_usb20hdc_set_dma_controller(endpoint, (unsigned long)request->request.dma + index, (unsigned long)endpoint->dma_transfer_buffer_address,
				      bytes, !(request->request.length - request->request.actual) ? 1 : 0);

	/* enable DMA transfer */
	f_usb20hdc_enable_dma_transfer(endpoint, 1U);

	fusb_func_trace("END 2");
	return 0;
}

static unsigned char f_usb20hdc_end_out_transfer_set_request_status(struct f_usb20hdc_udc_endpoint *endpoint, unsigned long byte)
{
	struct f_usb20hdc_udc_request *request  = endpoint->request;
	unsigned char endpoint_transfer_type    = endpoint->transfer_type;

	fusb_func_trace("START");

	if (request->request.length > request->request.actual) {
		if (endpoint->dma_transfer) {
			if (!get_dma_sp(endpoint->f_usb20hdc->register_base_address, endpoint->usb_dma_channel)) {
				fusb_func_trace("END 1a");
				return 0;
			}
		} else {
			if ((byte != 0) && !(byte % endpoint->endpoint.maxpacket)) {
				fusb_func_trace("END 1b");
				return 0;
			}
		}

		if ((endpoint_transfer_type == USB_ENDPOINT_XFER_CONTROL) && (byte == 8U)) {
			request->request.status = -EPROTO;
			fusb_func_trace("END 2");
			return 1U;
		}

		if (request->request.short_not_ok) {
			request->request.status = -EREMOTEIO;
			fusb_func_trace("END 3");
			return 1U;
		} else {
			fusb_func_trace("END 4");
			return 1U;
		}
	}
	else if (request->request.length == request->request.actual) {
		if (request->request.zero) {
			if ((byte != 0) && ((byte % endpoint->endpoint.maxpacket) == 0)) {
				fusb_func_trace("END 5");
				return 0;
			}
		}

		fusb_func_trace("END 6");
		return 1U;
	}
	else if (request->request.length < request->request.actual) {
		if (request->request.length % endpoint->endpoint.maxpacket) {
			if (byte <= endpoint->endpoint.maxpacket) {
				request->request.status = -EOVERFLOW;
				fusb_func_trace("END 7");
				return 1U;
			}
		} else {
			if (request->request.zero) {
				request->request.status = -EOVERFLOW;
				fusb_func_trace("END 8");
				return 1U;
			} else {
				if (endpoint_transfer_type == USB_ENDPOINT_XFER_CONTROL) {
					request->request.status = -EPROTO;
					fusb_func_trace("END 9");
					return 1U;
				} else {
					request->request.status = -EOVERFLOW;
					fusb_func_trace("END 10");
					return 1U;
				}
			}
		}
	}

	fusb_func_trace("END 11");
	return 0;
}

static unsigned char f_usb20hdc_set_out_transfer_pio(struct f_usb20hdc_udc_endpoint *endpoint);         /**< prototype */

static unsigned char f_usb20hdc_end_out_transfer_dma(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel;
	signed char usb_dma_channel = endpoint->usb_dma_channel;
	unsigned long bytes, dmatci, dmatc, phycnt, prev_actual;
	unsigned long *transfer_data;
	unsigned char index = get_appptr(f_usb20hdc->register_base_address, endpoint->endpoint_channel);

	endpoint_channel = endpoint->endpoint_channel;
	prev_actual = request->request.actual;

	/* get this time transfer byte */
	dmatc = get_dma_tc(f_usb20hdc->register_base_address, usb_dma_channel);
	dmatci = get_dma_tci(f_usb20hdc->register_base_address, usb_dma_channel);
	bytes = dmatc;

	/* check buffer overflow */
	if ((dmatc != dmatci) &&
	    ((dmatc % endpoint->endpoint.maxpacket) == 0) &&
	    !(get_dma_np(f_usb20hdc->register_base_address, usb_dma_channel))) {
		/* dma buffer overflow */
		phycnt = get_phycnt(f_usb20hdc->register_base_address,
				    endpoint_channel * F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT + index);
		bytes += phycnt;
	}

	/* update actual bytes */
	request->request.actual += bytes;

	/* check transfer request complete */
	if (f_usb20hdc_end_out_transfer_set_request_status(endpoint, bytes)) {
		/* complete request */
		f_usb20hdc_abort_out_transfer_dma(endpoint, 0);

		/* if buffer overflow is occured, DMA H/W cancels transfer.
		 * need pio transfer. */
		if (request->request.status == -EOVERFLOW) {
			/* calculate transfer data pointer */
			transfer_data = (unsigned long *)(request->request.buf + prev_actual);
			prefetch(transfer_data);
			/* read OUT transfer data from buffer */
			read_epbuf(f_usb20hdc->register_base_address, endpoint->buffer_address_offset[index], transfer_data, request->request.length - prev_actual);
			/* enable next OUT transfer */
			set_bufrd(f_usb20hdc->register_base_address, endpoint_channel);
			/* update actual bytes */
			request->request.actual = request->request.length;
		}

		return 1U;
	}

	if ((request->request.length == request->request.actual)
	    && ((bytes % endpoint->endpoint.maxpacket) == 0)
	    && (request->request.zero)) {
		f_usb20hdc_abort_out_transfer_dma(endpoint, 0);
		if (f_usb20hdc_set_out_transfer_pio(endpoint)) {
			endpoint->dma_transfer = 0;
			return 0;
		}
		request->request.status = -EPERM;
		return 1U;
	}

	/* calculate this time transfer byte */
	bytes = (request->request.length - request->request.actual) < F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES ?
		request->request.length : F_USB20HDC_UDC_CONFIG_DMA_TRANSFER_MAXIMUM_BYTES;
	if (bytes / endpoint->endpoint.maxpacket)
		bytes -= bytes % endpoint->endpoint.maxpacket ;
	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u DMA is setup at length = %u, actual = %u, max packet = %u, this time = %u.\n",
		endpoint_channel, request->request.length, request->request.actual, endpoint->endpoint.maxpacket, (unsigned int)bytes);

	/* setup DMA transfer */
	f_usb20hdc_set_dma_controller(endpoint, (unsigned long)endpoint->dma_transfer_buffer_address,
				      (unsigned long)request->request.dma + request->request.actual, bytes, 0);

	/* enable DMA transfer */
	f_usb20hdc_enable_dma_transfer(endpoint, 1U);

	return 0;
}

static unsigned char f_usb20hdc_is_dma_transfer_usable(struct f_usb20hdc_udc_endpoint *endpoint)
{
	unsigned int counter = 0;
	struct f_usb20hdc_udc_endpoint* ep;

	if ((endpoint->usb_dma_channel == -1) || (endpoint->dma_channel == -1) ||
	    (endpoint->dma_request_channel == DMA_REQUEST_NONE) || (endpoint->dma_transfer))
		return 0;

	for (counter = ENDPOINT_CHANNEL_ENDPOINT1; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		ep = &endpoint->f_usb20hdc->endpoint[counter];
		if (endpoint->endpoint_channel != ep->endpoint_channel) {
			if (ep->dma_transfer) {
				if ((endpoint->usb_dma_channel == ep->usb_dma_channel) ||
				    (endpoint->dma_channel == ep->dma_channel))
					return 0;
			}
		}
	}

	endpoint->dma_transfer = 1U;

	return 1U;
}

static unsigned char f_usb20hdc_is_pio_transfer_auto_change_usable(struct f_usb20hdc_udc_endpoint *endpoint)
{
	if ((endpoint->pio_transfer_auto_change) || (endpoint->usb_dma_channel == -1) ||
	    (endpoint->dma_channel == -1) || (endpoint->dma_request_channel == DMA_REQUEST_NONE))
		return 1;
	return 0;
}

static void f_usb20hdc_dequeue_all_transfer_request(struct f_usb20hdc_udc_endpoint *, int);             /**< prototype */

static irqreturn_t f_usb20hdc_on_end_dma_transfer(int irq, void *dev_id)
{
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc = dev_id;
	signed char dma_channel;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!dev_id))
		return IRQ_NONE;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);
	f_usb_spin_lock(f_usb20hdc);

	for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		dma_channel = f_usb20hdc->endpoint[counter].dma_channel;

		/* DMAC controller interrupt request assert check */
		if ((dma_channel == -1) || (irq != f_usb20hdc->dma_irq[dma_channel]))
			continue;

		/* DMA transfer end interrupt factor */
		switch (get_ss(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel)) {
		case SS_ADDRESS_OVERFLOW:
		case SS_TRANSFER_STOP_REQUEST:
		case SS_SOURCE_ACCESS_ERROR:
		case SS_DESTINATION_ACCESS_ERROR:
			/* clear DMA transfer end interrupt factor */
			clear_ss(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel);

			/* abort IN / OUT transfer */
			f_usb20hdc->endpoint[f_usb20hdc->dma_endpoint_channel[dma_channel]].transfer_direction ?
			f_usb20hdc_abort_in_transfer_dma(&f_usb20hdc->endpoint[f_usb20hdc->dma_endpoint_channel[dma_channel]], 1U) :
			f_usb20hdc_abort_out_transfer_dma(&f_usb20hdc->endpoint[f_usb20hdc->dma_endpoint_channel[dma_channel]], 1U);
			f_usb20hdc_dequeue_all_transfer_request(&f_usb20hdc->endpoint[f_usb20hdc->dma_endpoint_channel[dma_channel]], -EPERM);

			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

			f_usb_spin_unlock(f_usb20hdc);
			return IRQ_HANDLED;
		default:
			/* clear DMA transfer end interrupt factor */
			clear_ss(f_usb20hdc->dmac_register_base_address[dma_channel], dma_channel);
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

			f_usb_spin_unlock(f_usb20hdc);
			return IRQ_HANDLED;
		}
	}

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
	f_usb_spin_unlock(f_usb20hdc);

	fusb_func_trace("END");

	return IRQ_NONE;
}

/* So far, it is implementation of DMAC control local function for F_USB20HDC UDC device driver    */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HDC UDC device driver local function            */

static void f_usb20hdc_initialize_controller(struct f_usb20hdc_udc *f_usb20hdc, unsigned char preinitial)
{
	unsigned long counter;

	/*
	 * check host mode
	 * [notice]:It is processing nothing, when host_en bit is set
	 */
	if (get_host_en(f_usb20hdc->register_base_address))
		return;

	/* check pre-initalize */
	if (preinitial) {
		/* initialize F_USB20HDC system configuration register */
		set_soft_reset(f_usb20hdc->register_base_address);

		/*
		 * pre-initialize F_USB20HDC global interrupt register
		 * [notice]:otg_inten bit is always enable
		 */
		set_otg_inten(f_usb20hdc->register_base_address, 1U);

		return;
	}

	/*
	 * initialize F_USB20HDC system configuration register
	 * [notice]:set of soft_reset bit is prohibition
	 */
	set_byte_order(f_usb20hdc->register_base_address, 0);
	set_burst_wait(f_usb20hdc->register_base_address, 1U);

	/*
	 * initialize F_USB20HDC mode register
	 * [notice]:setup of dev_int_mode / dev_addr_load_mode bit is prohibition
	 */
	set_dev_en(f_usb20hdc->register_base_address, 0);

	/*
	 * initialize F_USB20HDC global interrupt register
	 * [notice]:otg_inten bit is always enable
	 */
	set_dev_inten(f_usb20hdc->register_base_address, 0);
	set_otg_inten(f_usb20hdc->register_base_address, 1U);
	set_phy_err_inten(f_usb20hdc->register_base_address, 0);
	set_cmd_inten(f_usb20hdc->register_base_address, 0);
	for (counter = USB_DMA_CHANNEL_DMA0; counter < F_USB20HDC_UDC_CONFIG_USB_DMA_CHANNELS; counter++)
		set_dma_inten(f_usb20hdc->register_base_address, counter, 0);
	for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++)
		set_dev_ep_inten(f_usb20hdc->register_base_address, counter, 0);
	clear_phy_err_int(f_usb20hdc->register_base_address);
	clear_cmd_int(f_usb20hdc->register_base_address);
	for (counter = USB_DMA_CHANNEL_DMA0; counter < F_USB20HDC_UDC_CONFIG_USB_DMA_CHANNELS; counter++)
		clear_dma_int(f_usb20hdc->register_base_address, counter);

	/* initialize F_USB20HDC device control / status / address register */
	if (f_usb20hdc->highspeed_support)
		set_reqspeed(f_usb20hdc->register_base_address, (fusb_otg_gadget_get_hs_enable() == USB_OTG_HS_DISABLE) ? REQ_SPEED_FULL_SPEED : REQ_SPEED_HIGH_SPEED);
	else
		set_reqspeed(f_usb20hdc->register_base_address, REQ_SPEED_FULL_SPEED);
	
	set_reqresume(f_usb20hdc->register_base_address, 0);
	set_enrmtwkup(f_usb20hdc->register_base_address, 0);
	set_suspende_inten(f_usb20hdc->register_base_address, 0);
	set_suspendb_inten(f_usb20hdc->register_base_address, 0);
	set_sof_inten(f_usb20hdc->register_base_address, 0);
	set_setup_inten(f_usb20hdc->register_base_address, 0);
	set_usbrste_inten(f_usb20hdc->register_base_address, 0);
	set_usbrstb_inten(f_usb20hdc->register_base_address, 0);
	set_status_ok_inten(f_usb20hdc->register_base_address, 0);
	set_status_ng_inten(f_usb20hdc->register_base_address, 0);
	clear_suspende_int(f_usb20hdc->register_base_address);
	clear_suspendb_int(f_usb20hdc->register_base_address);
	clear_sof_int(f_usb20hdc->register_base_address);
	clear_setup_int(f_usb20hdc->register_base_address);
	clear_usbrste_int(f_usb20hdc->register_base_address);
	clear_usbrstb_int(f_usb20hdc->register_base_address);
	clear_status_ok_int(f_usb20hdc->register_base_address);
	clear_status_ng_int(f_usb20hdc->register_base_address);
	set_func_addr(f_usb20hdc->register_base_address, 0);
	set_dev_configured(f_usb20hdc->register_base_address, 0);

	/* initialize F_USB20HDC dma register */
	for (counter = USB_DMA_CHANNEL_DMA0; counter < F_USB20HDC_UDC_CONFIG_USB_DMA_CHANNELS; counter++) {
		set_dma_st(f_usb20hdc->register_base_address,counter, 0);
		set_dma_mode(f_usb20hdc->register_base_address, counter, DMA_MDOE_DEMAND);
		set_dma_sendnull(f_usb20hdc->register_base_address, counter, 0);
		set_dma_int_empty(f_usb20hdc->register_base_address, counter, 0);
		set_dma_spr(f_usb20hdc->register_base_address, counter, 0);
		set_dma_ep(f_usb20hdc->register_base_address, counter, 0);
		set_dma_blksize(f_usb20hdc->register_base_address, counter, 0);
		set_dma_tci(f_usb20hdc->register_base_address, counter, 0);
	}

	/* initialize F_USB20HDC test control register */
	set_testp(f_usb20hdc->register_base_address, 0);
	set_testj(f_usb20hdc->register_base_address, 0);
	set_testk(f_usb20hdc->register_base_address, 0);
	set_testse0nack(f_usb20hdc->register_base_address, 0);

	/* initialize F_USB20HDC ram register */
	for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		clear_epctrl(f_usb20hdc->register_base_address, counter);
		clear_epconf(f_usb20hdc->register_base_address, counter);
	}
	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS * F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT; counter++)
		clear_epcount(f_usb20hdc->register_base_address, counter);

	/* wait PHY reset release */
	for (counter = 0xffffU; ((counter) && (get_phyreset(f_usb20hdc->register_base_address))); counter--);

	return;
}

static void f_usb20hdc_initialize_endpoint(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char fifo, unsigned char stall, unsigned char toggle)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	unsigned char endpoint_channel = endpoint->endpoint_channel;
	unsigned char endpoint_enable = get_ep_en(f_usb20hdc->register_base_address, endpoint_channel);

	fusb_func_trace("START");

	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
		/* check new SETUP transfer */
		if ((get_setup_int(f_usb20hdc->register_base_address)) || (get_usbrstb_int(f_usb20hdc->register_base_address)) ||
		    (get_usbrste_int(f_usb20hdc->register_base_address)) || (get_busreset(f_usb20hdc->register_base_address))) {
			fifo = 0;
			stall = 0;
			toggle = 0;
		}

		if (fifo) {
			if ((endpoint_enable) && (get_ep_en(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0)))
				/* disable endpoint */
				set_stop(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

			/* initialize FIFO */
			set_init(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		}

		if (stall) {
			/* initialize endpoint stall */
			set_stall_clear(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		}

		if (toggle) {
			/* initialize endpoint data toggle bit */
			set_toggle_clear(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		}

		/* initialize endpoint control / status register */
		if (fifo) {
			clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
			set_readyi_ready_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);
			set_readyo_empty_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);
		}
		set_ping_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);
		set_stalled_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 1U);
		set_nack_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);
		set_dev_ep_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 1U);

		if ((endpoint_enable) && (!get_ep_en(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0)))
			/* re-enable endpoint */
			set_start(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
	} else {
		if (fifo) {
			if ((endpoint_enable) && (get_ep_en(f_usb20hdc->register_base_address, endpoint_channel)))
				/* disable endpoint */
				set_stop(f_usb20hdc->register_base_address, endpoint_channel);

			/* initialize FIFO */
			set_init(f_usb20hdc->register_base_address, endpoint_channel);
		}

		if (stall) {
			/* initialize endpoint stall */
			set_stall_clear(f_usb20hdc->register_base_address, endpoint_channel);
		}

		if (toggle) {
			/* initialize endpoint data toggle bit */
			set_toggle_clear(f_usb20hdc->register_base_address, endpoint_channel);
		}

		/* initialize endpoint control / status register */
		clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, endpoint_channel);
		clear_readyo_empty_int_clr(f_usb20hdc->register_base_address, endpoint_channel);
		set_readyi_ready_inten(f_usb20hdc->register_base_address, endpoint_channel, 0);
		set_readyo_empty_inten(f_usb20hdc->register_base_address, endpoint_channel, 0);
		set_ping_inten(f_usb20hdc->register_base_address, endpoint_channel, 0);
		set_stalled_inten(f_usb20hdc->register_base_address, endpoint_channel, 1U);
		set_nack_inten(f_usb20hdc->register_base_address, endpoint_channel, 0);
		set_dev_ep_inten(f_usb20hdc->register_base_address, endpoint_channel, 1U);

		if ((endpoint_enable) && (!get_ep_en(f_usb20hdc->register_base_address, endpoint_channel)))
			/* re-enable endpoint */
			set_start(f_usb20hdc->register_base_address, endpoint_channel);
	}

	fusb_func_trace("END");
	return;
}

static void f_usb20hdc_initialize_endpoint_configure(struct f_usb20hdc_udc *f_usb20hdc)
{
	unsigned long counter, buffer_counter;
	unsigned char high_speed = f_usb20hdc->gadget_driver ? f_usb20hdc->gadget_driver->speed == USB_SPEED_HIGH ? 1U : 0 : 1U;

	/* initialzie endpoint 0 configure data */
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].endpoint.name = endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].name;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].endpoint.maxpacket =
		high_speed ? endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].maximum_packet_size_high_speed :
		endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].maximum_packet_size_full_speed;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].endpoint_channel = ENDPOINT_CHANNEL_ENDPOINT0;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].transfer_direction = 0;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].transfer_type = USB_ENDPOINT_XFER_CONTROL;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffer_address_offset[0] = get_epbuf_address_offset();
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffer_address_offset[1U] =
		f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffer_address_offset[0] + f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].endpoint.maxpacket;
	if (f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffer_address_offset[1U] & 0x3U)
		f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffer_address_offset[1U] +=
			4U - (f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffer_address_offset[1U] & 0x3U);
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffer_size = endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].buffer_size;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffers = endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].buffers;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].pio_transfer_auto_change = 0;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].in_transfer_end_timinig_host_transfer = 0;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].usb_dma_channel = endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].usb_dma_channel;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].dma_channel = endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].dma_channel;
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].dma_request_channel = endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].dma_request_channel;

	/* configure endpoint 0 */
	set_et(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, TYPE_CONTROL);
	set_dir(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);
	set_bnum(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 1U);
	set_hiband(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);
	set_base(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0,
		 f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].buffer_address_offset[0]);
	set_size(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0,
		 f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].endpoint.maxpacket);
	set_countidx(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);

	for (counter = ENDPOINT_CHANNEL_ENDPOINT1; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		/* initialzie endpoint configure data */
		f_usb20hdc->endpoint[counter].endpoint.name = endpoint_configuration_data[counter].name;
		f_usb20hdc->endpoint[counter].endpoint.maxpacket =
			high_speed ? endpoint_configuration_data[counter].maximum_packet_size_high_speed :
			endpoint_configuration_data[counter].maximum_packet_size_full_speed;
		f_usb20hdc->endpoint[counter].endpoint_channel = counter;
		f_usb20hdc->endpoint[counter].transfer_direction = 0;
		f_usb20hdc->endpoint[counter].transfer_type = 0;
		f_usb20hdc->endpoint[counter].buffer_address_offset[0] =
			f_usb20hdc->endpoint[counter - 1U].buffer_address_offset[F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT - 1U] +
			endpoint_configuration_data[counter - 1U].buffer_size;
		if (f_usb20hdc->endpoint[counter].buffer_address_offset[0] & 0x3U)
			f_usb20hdc->endpoint[counter].buffer_address_offset[0] += 4U - (f_usb20hdc->endpoint[counter].buffer_address_offset[0] & 0x3U);
		for (buffer_counter = 1U; buffer_counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT; buffer_counter++) {
			f_usb20hdc->endpoint[counter].buffer_address_offset[buffer_counter] =
				f_usb20hdc->endpoint[counter].buffer_address_offset[buffer_counter - 1U] + endpoint_configuration_data[counter].buffer_size;
			if (f_usb20hdc->endpoint[counter].buffer_address_offset[buffer_counter] & 0x3U)
				f_usb20hdc->endpoint[counter].buffer_address_offset[buffer_counter] +=
					4U - (f_usb20hdc->endpoint[counter].buffer_address_offset[buffer_counter] & 0x3U);
		}
		f_usb20hdc->endpoint[counter].buffer_size = endpoint_configuration_data[counter].buffer_size;
		f_usb20hdc->endpoint[counter].buffers = endpoint_configuration_data[counter].buffers;
		f_usb20hdc->endpoint[counter].pio_transfer_auto_change = endpoint_configuration_data[counter].pio_transfer_auto_change;
		f_usb20hdc->endpoint[counter].in_transfer_end_timinig_host_transfer = endpoint_configuration_data[counter].in_transfer_end_timinig_host_transfer;
		f_usb20hdc->endpoint[counter].usb_dma_channel = endpoint_configuration_data[counter].usb_dma_channel;
		f_usb20hdc->endpoint[counter].dma_channel = endpoint_configuration_data[counter].dma_channel;
		f_usb20hdc->endpoint[counter].dma_request_channel = endpoint_configuration_data[counter].dma_request_channel;
	}

	return;
}

static unsigned char f_usb20hdc_configure_endpoint(struct f_usb20hdc_udc_endpoint *endpoint)
{
	static const unsigned char transfer_type_register[] = {
		TYPE_CONTROL,		/* control transfer */
		TYPE_ISOCHRONOUS,	/* isochronout transfer */
		TYPE_BULK,		/* bulk transfer */
		TYPE_INTERRUPT, 	/* interrupt transfer */
	};
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	unsigned char endpoint_channel = endpoint->endpoint_descriptor->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	unsigned char endpoint_transfer_direction = endpoint->endpoint_descriptor->bEndpointAddress & USB_ENDPOINT_DIR_MASK ? 1U : 0;
	unsigned char endpoint_transfer_type = endpoint->endpoint_descriptor->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	unsigned short endpoint_transfer_packet_maximum_bytes = le16_to_cpu(endpoint->endpoint_descriptor->wMaxPacketSize) & 0x7ffU;
	unsigned short endpoint_hiband_width_packets = (le16_to_cpu(endpoint->endpoint_descriptor->wMaxPacketSize) >> 11) & 0x3U;
	unsigned short endpoint_buffer_address_offset = endpoint->buffer_address_offset[0];
	unsigned short endpoint_buffers = endpoint->buffers;

	/* check endpoint transfer buffer size */
	if (endpoint->buffer_size < endpoint_transfer_packet_maximum_bytes)
		return 0;

	/* check endpoint transfer packet maximum byte count violation */
	switch (endpoint_transfer_type) {
	case USB_ENDPOINT_XFER_CONTROL:
		if (((f_usb20hdc->gadget.speed == USB_SPEED_FULL) &&
		     (endpoint_transfer_packet_maximum_bytes % 8) && (endpoint_transfer_packet_maximum_bytes > 64U)) ||
		    ((f_usb20hdc->gadget.speed == USB_SPEED_HIGH) && (endpoint_transfer_packet_maximum_bytes != 64U)))
			return 0;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		if (((f_usb20hdc->gadget.speed == USB_SPEED_FULL) && (endpoint_transfer_packet_maximum_bytes > 1023U)) ||
		    ((f_usb20hdc->gadget.speed == USB_SPEED_HIGH) && (endpoint_transfer_packet_maximum_bytes > 1024U)))
			return 0;
		break;
	case USB_ENDPOINT_XFER_INT:
		if (((f_usb20hdc->gadget.speed == USB_SPEED_FULL) && (endpoint_transfer_packet_maximum_bytes > 64U)) ||
		    ((f_usb20hdc->gadget.speed == USB_SPEED_HIGH) && (endpoint_transfer_packet_maximum_bytes > 1024U)))
			return 0;
		break;
	case USB_ENDPOINT_XFER_BULK:
		if (((f_usb20hdc->gadget.speed == USB_SPEED_FULL) &&
		     (endpoint_transfer_packet_maximum_bytes % 8) && (endpoint_transfer_packet_maximum_bytes > 64U)) ||
		    ((f_usb20hdc->gadget.speed == USB_SPEED_HIGH) && (endpoint_transfer_packet_maximum_bytes != 512U)))
			return 0;
		break;
	default:
		return 0;
	}

	/* set endpoint configure data */
	endpoint->endpoint.maxpacket = endpoint_transfer_packet_maximum_bytes;
	endpoint->transfer_direction = endpoint_transfer_direction;
	endpoint->transfer_type = endpoint_transfer_type;
	for (counter = 1U; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT; counter++) {
		f_usb20hdc->endpoint[endpoint_channel].buffer_address_offset[counter] =
			f_usb20hdc->endpoint[endpoint_channel].buffer_address_offset[counter - 1U] + endpoint_transfer_packet_maximum_bytes;
		if (f_usb20hdc->endpoint[endpoint_channel].buffer_address_offset[counter] & 0x3U)
			f_usb20hdc->endpoint[endpoint_channel].buffer_address_offset[counter] +=
				4U - (f_usb20hdc->endpoint[endpoint_channel].buffer_address_offset[counter] & 0x3U);
	}

	/* configure endpoint x */
	set_et(f_usb20hdc->register_base_address, endpoint_channel, transfer_type_register[endpoint_transfer_type]);
	set_dir(f_usb20hdc->register_base_address, endpoint_channel, endpoint_transfer_direction ? 1U : 0);
	set_bnum(f_usb20hdc->register_base_address, endpoint_channel, endpoint_buffers);
	set_hiband(f_usb20hdc->register_base_address, endpoint_channel, endpoint_hiband_width_packets);
	set_base(f_usb20hdc->register_base_address, endpoint_channel, endpoint_buffer_address_offset);
	set_size(f_usb20hdc->register_base_address, endpoint_channel, endpoint_transfer_packet_maximum_bytes);
	set_countidx(f_usb20hdc->register_base_address, endpoint_channel, endpoint_channel * F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT);

	return 1U;
}

static unsigned char f_usb20hdc_is_endpoint_buffer_usable(void)
{
	unsigned long counter;
	unsigned long buffer_size = 0;

	/* calculate RAM buffer size */
	buffer_size += 256U;
	buffer_size += endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].buffer_size * 2U;
	for (counter = ENDPOINT_CHANNEL_ENDPOINT1; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++)
		buffer_size += endpoint_configuration_data[counter].buffer_size * endpoint_configuration_data[counter].buffers;

	return buffer_size <= F_USB20HDC_UDC_CONFIG_ENDPOINT_BUFFER_RAM_SIZE ? 1U : 0;
}

static void f_usb20hdc_enable_endpoint(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char enable)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	unsigned char endpoint_channel = endpoint->endpoint_channel;

	if (enable) {
		/* initialize endpoint */
		f_usb20hdc_initialize_endpoint(endpoint, 1U, 1U, 1U);

		/* set endpoint parameter */
		endpoint->halt = 0;
		endpoint->null_packet_transfer = 0;

		/* enable endpoint */
		set_start(f_usb20hdc->register_base_address, endpoint_channel);
	} else {
		/*
		 * [notice]:use of an abort_xx_transfer() is prohibition, because cache becomes panic.
		 */

		/* disable DMA transfer */
		f_usb20hdc_enable_dma_transfer(endpoint, 0);

		/* disable endpoint */
		set_stop(f_usb20hdc->register_base_address, endpoint_channel);

		/* initialize endpoint */
		f_usb20hdc_initialize_endpoint(endpoint, 1U, 1U, 1U);

		/* set endpoint parameter */
		endpoint->halt = 1U;
		endpoint->force_halt = 0;
		endpoint->null_packet_transfer = 0;
	}

	return;
}

static void f_usb20hdc_set_bus_speed(struct f_usb20hdc_udc *f_usb20hdc)
{
	/* set current bus speed */
	f_usb20hdc->gadget.speed = get_crtspeed(f_usb20hdc->register_base_address) == CRT_SPEED_HIGH_SPEED ?
				   USB_SPEED_HIGH : USB_SPEED_FULL;

	/* set endpoint 0 max packet */
	f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].endpoint.maxpacket =
		f_usb20hdc->gadget.speed == USB_SPEED_HIGH ?
		endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].maximum_packet_size_high_speed :
		endpoint_configuration_data[ENDPOINT_CHANNEL_ENDPOINT0].maximum_packet_size_full_speed;
	set_size(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0,
		 f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0].endpoint.maxpacket);

	return;
}

static void f_usb20hdc_set_device_state(struct f_usb20hdc_udc *f_usb20hdc, unsigned char device_state)
{
	if (f_usb20hdc->device_state == device_state)
		return;

	/* set device state */
	f_usb20hdc->device_state_last = f_usb20hdc->device_state;
	f_usb20hdc->device_state = device_state;
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "device state is is %u.\n", device_state);

	return;
}

static void f_usb20hdc_notify_transfer_request_complete(struct f_usb20hdc_udc_endpoint *endpoint, int status)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel;
	unsigned char halt = endpoint->halt;

	fusb_func_trace("START");

	if (unlikely(!request))
		return;

	f_usb20hdc = endpoint->f_usb20hdc;

	endpoint_channel = endpoint->endpoint_channel;

	/* delete and initialize list */
	list_del_init(&request->queue);
	endpoint->request = NULL;

	/* set request status */
	if (request->request.status == -EINPROGRESS)
		request->request.status = status;
	else
		status = request->request.status;

	/* clear request execute */
	request->request_execute = 0;

	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u request is completed at request = 0x%p, length = %u, actual = %u, status = %d.\n",
		endpoint_channel, &request->request, request->request.length, request->request.actual, status);

	/* check request complete notify for gadget driver */
	if ((request->request.no_interrupt) || (!request->request.complete))
		return;

	/* notify request complete for gadget driver */
	endpoint->halt = 1U;

	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	request->request.complete(&endpoint->endpoint, &request->request);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);

	endpoint->halt = halt;

	fusb_func_trace("END");
	return;
}

static void f_usb20hdc_dequeue_all_transfer_request(struct f_usb20hdc_udc_endpoint *endpoint, int status)
{
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request;
	void *dma_virtual_address;

	/* dequeue all transfer request */
	for (counter = (unsigned long)-1; (counter) && (!list_empty(&endpoint->queue)); counter--) {
		request = list_entry(endpoint->queue.next, struct f_usb20hdc_udc_request, queue);
		/* check DMA transfer buffer unmap */
		if ((endpoint->dma_transfer) && (request->request.dma != ~(dma_addr_t)0)) {
			if (request->dma_transfer_buffer_map) {
				/* unmap DMA transfer buffer and synchronize DMA transfer buffer */
				f_usb_dma_unmap_single(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length,
						 endpoint->transfer_direction ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
				f_usb_dma_sync_single_for_cpu(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length,
							endpoint->transfer_direction ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
				request->request.dma = ~(dma_addr_t)0;
				request->dma_transfer_buffer_map = 0;
			} else {
				dma_virtual_address = dma_to_virt(f_usb20hdc->gadget.dev.parent, request->request.dma);
				/* check DMA transfer buffer synchronize */
				if ((virt_addr_valid(dma_virtual_address)) && (virt_addr_valid(dma_virtual_address + request->request.length - 1U)) &&
				    (!((VMALLOC_START <= (unsigned long)request->request.buf) && ((unsigned long)request->request.buf <= VMALLOC_END))))
					/* synchronize DMA transfer buffer for CPU */
					f_usb_dma_sync_single_for_cpu(f_usb20hdc->gadget.dev.parent, request->request.dma, request->request.length,
								endpoint->transfer_direction ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
			}
		}
		endpoint->request = request;
		f_usb20hdc_notify_transfer_request_complete(endpoint, status);
	}

	return;
}

static unsigned char f_usb20hdc_is_setup_transferred(struct f_usb20hdc_udc *f_usb20hdc)
{
	return (get_setup_int(f_usb20hdc->register_base_address)) || (get_usbrstb_int(f_usb20hdc->register_base_address)) ||
	       (get_usbrste_int(f_usb20hdc->register_base_address)) || (get_busreset(f_usb20hdc->register_base_address)) ? 1U : 0;
}

static unsigned char f_usb20hdc_set_in_transfer_pio(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel = endpoint->endpoint_channel;
	unsigned long *transfer_data;
	unsigned char index = endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ? 0 : get_appptr(f_usb20hdc->register_base_address, endpoint_channel);
	unsigned long bytes;

	/* check argument */
	if (unlikely((endpoint_channel != ENDPOINT_CHANNEL_ENDPOINT0) && (!endpoint->endpoint_descriptor)))
		return 0;

	/* check new SETUP transfer */
	if ((endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) && (f_usb20hdc_is_setup_transferred(f_usb20hdc))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():new SETUP tranfer is occurred.\n", __FUNCTION__);
		return 0;
	}

	/* check transfer data setup */
	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ?
	    get_fulli(f_usb20hdc->register_base_address) :
	    get_fullo_full(f_usb20hdc->register_base_address, endpoint_channel)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u buffer is full.\n", __FUNCTION__, endpoint_channel);
		return 0;
	}

	/* check NULL packet IN transfer for last packet of transaction */
	if ((request->request.length) && (!(request->request.length % endpoint->endpoint.maxpacket)) && (request->request.zero))
		endpoint->null_packet_transfer = 1U;

	/* calculate transfer data pointer */
	transfer_data = (unsigned long *)(request->request.buf + request->request.actual);
	prefetch(transfer_data);

	/* calculate this time transfer byte */
	bytes = (request->request.length - request->request.actual) < endpoint->endpoint.maxpacket ?
		request->request.length - request->request.actual : endpoint->endpoint.maxpacket;
	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u PIO is setup at length = %u, actual = %u, max packet = %u, this time = %u.\n",
		endpoint_channel, request->request.length, request->request.actual,
		endpoint->endpoint.maxpacket, (unsigned int)bytes);

	/* update actual byte */
	request->request.actual = bytes;

	/* set request execute */
	request->request_execute = 1U;

	/* check buffer write bytes */
	if (bytes)
		/* write IN transfer data to buffer */
		write_epbuf(f_usb20hdc->register_base_address, endpoint->buffer_address_offset[index], transfer_data, bytes);

	/* notify buffer write bytes */
	set_appcnt(f_usb20hdc->register_base_address, endpoint_channel * F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT + index, bytes);

	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
		/* check new SETUP transfer */
		if (!f_usb20hdc_is_setup_transferred(f_usb20hdc)) {
			/* enable IN transfer & readyi interrupt */
			set_bufwr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
			set_readyi_ready_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 1U);
		}
	} else {
		/* check IN transfer end timing and endpoint buffer count */
		if ((endpoint->in_transfer_end_timinig_host_transfer) && (endpoint->buffers >= 2U))
			/*
			 * clear empty interrupt factor
			 * [notice]:It is mandatory processing.
			 */
			clear_readyo_empty_int_clr(f_usb20hdc->register_base_address, endpoint_channel);

		/* check DMA transfer usable */
		if (endpoint->dma_channel != -1)
			/*
			 * clear ready interrupt factor
			 * [notice]:It is mandatory processing.
			 */
			clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, endpoint_channel);

		/* enable IN transfer & ready interrupt */
		set_bufwr(f_usb20hdc->register_base_address, endpoint_channel);
		set_readyi_ready_inten(f_usb20hdc->register_base_address, endpoint_channel, 1U);
	}

	return 1U;
}

static unsigned char f_usb20hdc_set_out_transfer_pio(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel = endpoint->endpoint_channel;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely((endpoint_channel != ENDPOINT_CHANNEL_ENDPOINT0) && (!endpoint->endpoint_descriptor)))
		return 0;

	/* check new SETUP transfer */
	if ((endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) && (f_usb20hdc_is_setup_transferred(f_usb20hdc))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():new SETUP tranfer is occurred.\n", __FUNCTION__);
		return 0;
	}

	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u PIO is setup at length = %u, actual = %u, max packet = %u.\n",
		endpoint_channel, request->request.length, request->request.actual, endpoint->endpoint.maxpacket);

	/* set request execute */
	request->request_execute = 1U;

	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
		/* check new SETUP transfer */
		if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
			/* enable readyo interrupt */
			set_readyo_empty_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 1U);
	} else {
		/* check DMA transfer usable */
		if (endpoint->dma_channel != -1) {
			/*
			 * clear ready interrupt factor
			 * [notice]:It is mandatory processing.
			 */
			set_nackresp(f_usb20hdc->register_base_address, endpoint_channel, 1U);
			if (get_emptyo_empty(f_usb20hdc->register_base_address, endpoint_channel))
				clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, endpoint_channel);
			set_nackresp(f_usb20hdc->register_base_address, endpoint_channel, 0);
		}

		/* enable ready interrupt */
		set_readyi_ready_inten(f_usb20hdc->register_base_address, endpoint_channel, 1U);
	}

	fusb_func_trace("END");
	return 1U;
}

static void f_usb20hdc_abort_in_transfer_pio(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char initalize)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	unsigned char endpoint_channel = endpoint->endpoint_channel;

	/* disable endpoint interrupt */
	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
		set_readyi_ready_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);
	} else {
		set_readyi_ready_inten(f_usb20hdc->register_base_address, endpoint_channel, 0);
		set_readyo_empty_inten(f_usb20hdc->register_base_address, endpoint_channel, 0);
	}

	/* clear NULL packet transfer */
	endpoint->null_packet_transfer = 0;

	if (!initalize)
		return;

	/* initialize endpoint */
	f_usb20hdc_initialize_endpoint(endpoint, 1U, 0, 0);

	return;
}

static void f_usb20hdc_abort_out_transfer_pio(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char initalize)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	unsigned char endpoint_channel = endpoint->endpoint_channel;

	/* disable endpoint interrupt */
	endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ?
	set_readyo_empty_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0) :
	set_readyi_ready_inten(f_usb20hdc->register_base_address, endpoint_channel, 0);

	if (!initalize)
		return;

	/* initialize endpoint */
	f_usb20hdc_initialize_endpoint(endpoint, 1U, 0, 0);

	return;
}

static unsigned char f_usb20hdc_end_in_transfer_pio(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel = endpoint->endpoint_channel;
	unsigned long *transfer_data;
	unsigned char index = endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ? 0 : get_appptr(f_usb20hdc->register_base_address, endpoint_channel);
	unsigned long bytes;

	fusb_func_trace("START");

	/* check empty wait */
	if ((endpoint_channel != ENDPOINT_CHANNEL_ENDPOINT0) && (get_readyo_empty_inten(f_usb20hdc->register_base_address, endpoint_channel))) {
		/* complete request */
		f_usb20hdc_abort_in_transfer_pio(endpoint, 0);
		fusb_func_trace("END 1");
		return 1U;
	}

	/* check transfer remain byte */
	if (!(request->request.length - request->request.actual)) {
		/* check NULL packet IN transfer for last packet of transaction */
		if (endpoint->null_packet_transfer) {
			if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
				/* check new SETUP transfer */
				if (f_usb20hdc_is_setup_transferred(f_usb20hdc)) {
					f_usb20hdc_abort_in_transfer_pio(endpoint, 0);
					fusb_func_trace("END 2");
					return 0;
				}
				/* notify buffer write bytes */
				set_appcnt(f_usb20hdc->register_base_address, 0, 0);

				/* enable IN transfer */
				set_bufwr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
			} else {
				/* notify buffer write bytes */
				set_appcnt(f_usb20hdc->register_base_address,
					   endpoint_channel * F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT + index, 0);

				/* enable IN transfer */
				set_bufwr(f_usb20hdc->register_base_address, endpoint_channel);
			}
			endpoint->null_packet_transfer = 0;
			fusb_func_trace("END 3");
			return 0;
		} else {
			/* check IN transfer end timing and endpoint buffer count */
			if ((endpoint->in_transfer_end_timinig_host_transfer) && (endpoint->buffers >= 2U)) {
				/* enable empty interrupt */
				set_readyo_empty_inten(f_usb20hdc->register_base_address, endpoint_channel, 1U);
				return 0;
			}

			/* complete request */
			f_usb20hdc_abort_in_transfer_pio(endpoint, 0);
			fusb_func_trace("END 4");
			return 1U;
		}
	}

	/* check transfer data setup */
	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ?
	    get_fulli(f_usb20hdc->register_base_address) :
	    get_fullo_full(f_usb20hdc->register_base_address, endpoint_channel)) {
		/* abort IN transfer */
		f_usb20hdc_abort_in_transfer_pio(endpoint, endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ? 0 : 1U);
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u buffer is full.\n", __FUNCTION__, endpoint_channel);
		fusb_func_trace("END 5");
		return 0;
	}

	/* calculate transfer data pointer */
	transfer_data = (unsigned long *)(request->request.buf + request->request.actual);
	prefetch(transfer_data);

	/* calculate this time transfer byte */
	bytes = (request->request.length - request->request.actual) < endpoint->endpoint.maxpacket ?
		request->request.length - request->request.actual : endpoint->endpoint.maxpacket;
	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u PIO is setup at length = %u, actual = %u, max packet = %u, this time = %u.\n",
		endpoint_channel, request->request.length, request->request.actual, endpoint->endpoint.maxpacket, (unsigned int)bytes);

	/* update actual bytes */
	request->request.actual += bytes;

	/* check buffer write bytes */
	if (bytes)
		/* write IN transfer data to buffer */
		write_epbuf(f_usb20hdc->register_base_address, endpoint->buffer_address_offset[index], transfer_data, bytes);

	/* notify buffer write bytes */
	set_appcnt(f_usb20hdc->register_base_address, endpoint_channel * F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT + index, bytes);

	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
		/* check new SETUP transfer */
		if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
			/* enable IN transfer */
			set_bufwr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
	} else {
		/* enable IN transfer */
		set_bufwr(f_usb20hdc->register_base_address, endpoint_channel);
	}
	fusb_func_trace("END 6");
	return 0;
}

static unsigned char f_usb20hdc_end_out_transfer_pio(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char endpoint_channel = endpoint->endpoint_channel;
	unsigned long *transfer_data;
	unsigned char index = endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ? 1U : get_appptr(f_usb20hdc->register_base_address, endpoint_channel);
	unsigned long bytes, phycnt;

	fusb_func_trace("START");

	/* check transfer data read enable */
	if (get_emptyo_empty(f_usb20hdc->register_base_address, endpoint_channel)) {
		/* abort OUT transfer */
		f_usb20hdc_abort_out_transfer_pio(endpoint, endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0 ? 0 : 1U);
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u is empty.\n", __FUNCTION__, endpoint_channel);

		/* check new SETUP transfer */
		if ((endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) && (f_usb20hdc_is_setup_transferred(f_usb20hdc)) && (!request->request.length)) {
			request->request.actual = 0;
			fusb_func_trace("END 1");
			return 1U;
		}
		fusb_func_trace("END 2");
		return 0;
	}

	/* calculate transfer data pointer */
	transfer_data = (unsigned long *)(request->request.buf + request->request.actual);
	prefetch(transfer_data);

	/* get OUT transfer byte */
	bytes = phycnt = get_phycnt(f_usb20hdc->register_base_address, endpoint_channel * F_USB20HDC_UDC_CONFIG_ENDPOINT_COUNTERS_PER_ONE_ENDPOINT + index);
	if (request->request.length < (request->request.actual + bytes))
		bytes = request->request.length - request->request.actual;
	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u PIO is end at length = %u, actual = %u, max packet = %u, this time = %u.\n",
		endpoint_channel, request->request.length, request->request.actual, endpoint->endpoint.maxpacket, (unsigned int)bytes);

	/* update actual bytes */
	request->request.actual += phycnt;

	/* check buffer read bytes */
	if (bytes)
		/* read OUT transfer data form buffer */
		read_epbuf(f_usb20hdc->register_base_address, endpoint->buffer_address_offset[index], transfer_data, bytes);

	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
		/* check new SETUP transfer */
		if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
			/* enable next OUT transfer */
			set_bufrd(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
	} else {
		/* enable next OUT transfer */
		set_bufrd(f_usb20hdc->register_base_address, endpoint_channel);
	}

	/* check transfer request complete */
	if (f_usb20hdc_end_out_transfer_set_request_status(endpoint, phycnt)) {
		/* complete request */
		f_usb20hdc_abort_out_transfer_pio(endpoint, 0);

		if (request->request.status == -EOVERFLOW)
			/* update actual bytes */
			request->request.actual = request->request.length;

		fusb_func_trace("END 3");
		return 1U;
	}
	fusb_func_trace("END 4");
	return 0;
}

static unsigned char f_usb20hdc_set_in_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	static unsigned char (* const set_in_transfer_function[])(struct f_usb20hdc_udc_endpoint *) = {
		f_usb20hdc_set_in_transfer_pio,
		f_usb20hdc_set_in_transfer_dma,
	};
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char index = 1U;
	unsigned char result;

	/* check NULL packet transfer */
	if (!request->request.length)
		return f_usb20hdc_set_in_transfer_pio(endpoint);

	/* check DMA transfer usable */
	if (!f_usb20hdc_is_dma_transfer_usable(endpoint)) {
		if (!f_usb20hdc_is_pio_transfer_auto_change_usable(endpoint))
			return 0;
		index = 0;
	}

	/* get IN transfer process result */
	result = set_in_transfer_function[index](endpoint);

	/* check PIO transfer auto change usable */
	if ((!result) && (index == 1) && (f_usb20hdc_is_pio_transfer_auto_change_usable(endpoint)))
		return f_usb20hdc_set_in_transfer_pio(endpoint);

	return result;
}

static unsigned char f_usb20hdc_set_out_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	static unsigned char (* const set_out_transfer_function[])(struct f_usb20hdc_udc_endpoint *) = {
		f_usb20hdc_set_out_transfer_pio,
		f_usb20hdc_set_out_transfer_dma,
	};
	struct f_usb20hdc_udc_request *request = endpoint->request;
	unsigned char index = 1U;
	unsigned char result;

	/* check NULL packet transfer */
	if (!request->request.length)
		return f_usb20hdc_set_out_transfer_pio(endpoint);

	/* check DMA transfer usable */
	if (!f_usb20hdc_is_dma_transfer_usable(endpoint)) {
		if (!f_usb20hdc_is_pio_transfer_auto_change_usable(endpoint))
			return 0;
		index = 0;
	}

	/* get OUT transfer process result */
	result = set_out_transfer_function[index](endpoint);

	/* check PIO transfer auto change usable */
	if ((!result) && (index == 1) && (f_usb20hdc_is_pio_transfer_auto_change_usable(endpoint)))
		return f_usb20hdc_set_out_transfer_pio(endpoint);

	return result;
}

static void f_usb20hdc_abort_in_transfer(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char initalize)
{
	static void (* const abort_in_transfer_function[])(struct f_usb20hdc_udc_endpoint *, unsigned char) = {
		f_usb20hdc_abort_in_transfer_pio,
		f_usb20hdc_abort_in_transfer_dma,
	};
	return abort_in_transfer_function[endpoint->dma_transfer ? 1U : 0](endpoint, initalize);
}

static void f_usb20hdc_abort_out_transfer(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char initalize)
{
	static void (* const abort_out_transfer_function[])(struct f_usb20hdc_udc_endpoint *, unsigned char) = {
		f_usb20hdc_abort_out_transfer_pio,
		f_usb20hdc_abort_out_transfer_dma,
	};
	return abort_out_transfer_function[endpoint->dma_transfer ? 1U : 0](endpoint, initalize);
}

static unsigned char f_usb20hdc_end_in_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	static unsigned char (* const end_in_transfer_function[])(struct f_usb20hdc_udc_endpoint *) = {
		f_usb20hdc_end_in_transfer_pio,
		f_usb20hdc_end_in_transfer_dma,
	};
	return end_in_transfer_function[endpoint->dma_transfer ? 1U : 0](endpoint);
}

static unsigned char f_usb20hdc_end_out_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	static unsigned char (* const end_out_transfer_function[])(struct f_usb20hdc_udc_endpoint *) = {
		f_usb20hdc_end_out_transfer_pio,
		f_usb20hdc_end_out_transfer_dma,
	};
	return end_out_transfer_function[endpoint->dma_transfer ? 1U : 0](endpoint);
}

static void f_usb20hdc_halt_transfer(struct f_usb20hdc_udc_endpoint *endpoint, unsigned char halt, unsigned char force_halt)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct f_usb20hdc_udc_request *request;
	unsigned char endpoint_channel = endpoint->endpoint_channel;

	/* check isochronous endpoint */
	if (endpoint->transfer_type == USB_ENDPOINT_XFER_ISOC)
		return;

	if (halt) {
		if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
			/* check new SETUP transfer */
			if (f_usb20hdc_is_setup_transferred(f_usb20hdc))
				return;
		}

		/* set transfer halt */
		endpoint->halt = 1U;
		if (force_halt)
			endpoint->force_halt = 1U;

		/* check endpoint x halt clear */
		if (!get_stall(f_usb20hdc->register_base_address, endpoint_channel))
			/* halt endpoint x */
			set_stall_set(f_usb20hdc->register_base_address, endpoint_channel);
	} else {
		/* check force halt */
		if (endpoint->force_halt) {
			/* always clear endpoint x data toggle bit */
			set_toggle_clear(f_usb20hdc->register_base_address, endpoint_channel);
			return;
		}

		/* check endpoint x halt */
		if (get_stall(f_usb20hdc->register_base_address, endpoint_channel))
			/* clear endpoint x halt */
			set_stall_clear(f_usb20hdc->register_base_address, endpoint_channel);

		/* always clear endpoint x data toggle bit */
		set_toggle_clear(f_usb20hdc->register_base_address, endpoint_channel);

		/* clear transfer halt */
		endpoint->halt = 0;

		/* check next queue empty */
		if (list_empty(&endpoint->queue))
			return;

		/* get next request */
		request = list_entry(endpoint->queue.next, struct f_usb20hdc_udc_request, queue);

		/* check the got next request a request under current execution */
		if (request->request_execute)
			return;

		/* save request */
		endpoint->request = request;

		/* set endpoint x transfer request */
		if (!(endpoint->transfer_direction ? f_usb20hdc_set_in_transfer(endpoint) : f_usb20hdc_set_out_transfer(endpoint))) {
			hdc_err(f_usb20hdc->gadget.dev.parent, "%s():%s of endpoint %u is failed.\n",
				__FUNCTION__, endpoint->transfer_direction ? "f_usb20hdc_set_in_transfer()" : "f_usb20hdc_set_out_transfer()", endpoint_channel);
			f_usb20hdc_dequeue_all_transfer_request(endpoint, -EPERM);
		}
	}

	return;
}

static unsigned char f_usb20hdc_respond_standard_request_get_status(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup)) {
		return 0;
	}

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			 __FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1U : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_clear_feature(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	fusb_func_trace("START");

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup)) {
		fusb_func_trace("END 1");
		return 0;
	}

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			 __FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		fusb_func_trace("END 2");
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1U : 0;
	}
	fusb_func_trace("END");
	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_set_feature(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup)) {
		return 0;
	}

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			 __FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1U : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_set_address(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;

	/* check reqest target and request parameter */
	if (((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_DEVICE) || (ctrlrequest.bRequestType & USB_DIR_IN) ||
	    (ctrlrequest.wIndex) || (ctrlrequest.wLength))
		return 0;

	/* check device state */
	switch (f_usb20hdc->device_state) {
	case USB_STATE_DEFAULT:
	case USB_STATE_ADDRESS:
		break;
	default:
		return 0;
	}

	/* set address value */
	set_func_addr(f_usb20hdc->register_base_address, ctrlrequest.wValue & 0xff);

	/* change device state */
	f_usb20hdc_set_device_state(f_usb20hdc, USB_STATE_ADDRESS);

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_get_descriptor(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check reqest target and request parameter */
	if ((((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_DEVICE) &&
	     ((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)) ||
	     (!(ctrlrequest.bRequestType & USB_DIR_IN)) ||
	     (((ctrlrequest.wValue >> 8) != USB_DT_STRING) && (ctrlrequest.wIndex)))
		return 0;

	/* check device state */
	switch (f_usb20hdc->device_state) {
	case USB_STATE_DEFAULT:
	case USB_STATE_ADDRESS:
	case USB_STATE_CONFIGURED:
		break;
	default:
		return 0;
	}

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		dev_dbg(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1 : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_set_descriptor(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check reqest target and request parameter */
	if ((((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_DEVICE) &&
	     ((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)) ||
	     (ctrlrequest.bRequestType & USB_DIR_IN) ||
	     (((ctrlrequest.wValue >> 8) != USB_DT_STRING) && (ctrlrequest.wIndex)))
		return 0;

	/* check device state */
	switch (f_usb20hdc->device_state) {
	case USB_STATE_ADDRESS:
	case USB_STATE_CONFIGURED:
		break;
	default:
		return 0;
	}

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		dev_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1 : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_get_configuration(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check reqest target and request parameter */
	if (((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_DEVICE) || (!(ctrlrequest.bRequestType & USB_DIR_IN)) ||
	    (ctrlrequest.wValue) || (ctrlrequest.wIndex) || (ctrlrequest.wLength != 1))
		return 0;

	/* check device state */
	switch (f_usb20hdc->device_state) {
	case USB_STATE_ADDRESS:
	case USB_STATE_CONFIGURED:
		/* check SETUP transfer callback to gadget driver */
		if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
			return 0;

		/* notify SETUP transfer to gadget driver */
		if (f_usb20hdc->lock_cnt > 0)
			spin_unlock(&f_usb20hdc->lock);
		result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
		if (f_usb20hdc->lock_cnt > 0)
			spin_lock(&f_usb20hdc->lock);
		if (result < 0) {
			hdc_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
				__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
			return 0;
		}

		/* check gadget driver processing result */
		if (result) {
			/* delay NULL packet transfer for status stage */
			f_usb20hdc->control_transfer_status_stage_delay = 1U;
			f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1U : 0;
		}
		break;
	default:
		return 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_set_configuration(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	unsigned char configure_value = ctrlrequest.wValue & 0xff;
	unsigned char configure_value_last = f_usb20hdc->configure_value_last;
	int result;

	/* check reqest target and request parameter */
	if (((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_DEVICE) || (ctrlrequest.bRequestType & USB_DIR_IN) ||
	    (ctrlrequest.wIndex) || (ctrlrequest.wLength))
		return 0;

	/* check device state */
	switch (f_usb20hdc->device_state) {
	case USB_STATE_ADDRESS:
	case USB_STATE_CONFIGURED:
		break;
	default:
		return 0;
	}

	/* check configure value */
	if (configure_value) {
		/* check configure value change */
		if ((configure_value != f_usb20hdc->configure_value_last) && (f_usb20hdc->configure_value_last)) {
			f_usb20hdc->configure_value_last = configure_value;
			for (counter = ENDPOINT_CHANNEL_ENDPOINT1; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++)
				/* abort transfer */
				f_usb20hdc->endpoint[counter].transfer_direction ?
				f_usb20hdc_abort_in_transfer(&f_usb20hdc->endpoint[counter], 1) : f_usb20hdc_abort_out_transfer(&f_usb20hdc->endpoint[counter], 1);
		}

		/* change device state */
		f_usb20hdc_set_device_state(f_usb20hdc, USB_STATE_CONFIGURED);
	} else {
		/* check configure value change */
		if (configure_value != f_usb20hdc->configure_value_last) {
			f_usb20hdc->configure_value_last = configure_value;
			for (counter = ENDPOINT_CHANNEL_ENDPOINT1; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++)
				/* abort transfer */
				f_usb20hdc->endpoint[counter].transfer_direction ?
				f_usb20hdc_abort_in_transfer(&f_usb20hdc->endpoint[counter], 1) : f_usb20hdc_abort_out_transfer(&f_usb20hdc->endpoint[counter], 1);
		}

		/* change device state */
		f_usb20hdc_set_device_state(f_usb20hdc, USB_STATE_ADDRESS);
	}

	/* dequeue all previous transfer request */
	for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++)
		f_usb20hdc_dequeue_all_transfer_request(&f_usb20hdc->endpoint[counter], -ECONNABORTED);

	/* set configuration value */
	set_dev_configured(f_usb20hdc->register_base_address, configure_value ? 1U : 0);

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		f_usb20hdc->configure_value_last = configure_value_last;
		f_usb20hdc_set_device_state(f_usb20hdc, configure_value_last ? USB_STATE_CONFIGURED : USB_STATE_ADDRESS);
		set_dev_configured(f_usb20hdc->register_base_address, configure_value_last ? 1 : 0);
		dev_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1 : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_get_interface(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check reqest target and request parameter */
	if (((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE) || (!(ctrlrequest.bRequestType & USB_DIR_IN)) ||
	    (ctrlrequest.wValue) || (ctrlrequest.wLength != 1))
		return 0;

	/* check device state */
	switch (f_usb20hdc->device_state) {
	case USB_STATE_CONFIGURED:
		break;
	default:
		return 0;
	}

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		dev_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1 : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_set_interface(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check reqest target and request parameter */
	if (((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE) || (ctrlrequest.bRequestType & USB_DIR_IN) ||
	    (ctrlrequest.wLength))
		return 0;

	/* check device state */
	switch (f_usb20hdc->device_state) {
	case USB_STATE_CONFIGURED:
		break;
	default:
		return 0;
	}

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		dev_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1 : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_sync_frame(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check reqest target and request parameter */
	if (((ctrlrequest.bRequestType & USB_RECIP_MASK) != USB_RECIP_ENDPOINT) || (!(ctrlrequest.bRequestType & USB_DIR_IN)) ||
	    (ctrlrequest.wValue) || (ctrlrequest.wLength != 2U))
		return 0;

	/* check device state */
	switch (f_usb20hdc->device_state) {
	case USB_STATE_CONFIGURED:
		break;
	default:
		return 0;
	}

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		dev_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1 : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_standard_request_undefined(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1U : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_class_request(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1U : 0;
	}

	return 1U;
}

static unsigned char f_usb20hdc_respond_vendor_request(struct f_usb20hdc_udc_endpoint *endpoint, struct usb_ctrlrequest ctrlrequest)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	int result;

	/* check SETUP transfer callback to gadget driver */
	if ((!f_usb20hdc->gadget_driver) || (!f_usb20hdc->gadget_driver->setup))
		return 0;

	/* notify SETUP transfer to gadget driver */
	if (f_usb20hdc->lock_cnt > 0)
		spin_unlock(&f_usb20hdc->lock);
	result = f_usb20hdc->gadget_driver->setup(&f_usb20hdc->gadget, &ctrlrequest);
	if (f_usb20hdc->lock_cnt > 0)
		spin_lock(&f_usb20hdc->lock);
	if (result < 0) {
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s():%s of setup() is failed at %d.\n",
			__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
		return 0;
	}

	/* check gadget driver processing result */
	if (result) {
		/* delay NULL packet transfer for status stage */
		f_usb20hdc->control_transfer_status_stage_delay = 1U;
		f_usb20hdc->control_transfer_priority_direction = (ctrlrequest.bRequestType & USB_DIR_IN) || (!ctrlrequest.wLength) ? 1U : 0;
	}

	return 1U;
}

static void f_usb20hdc_enable_host_connect(struct f_usb20hdc_udc *f_usb20hdc, unsigned char enable)
{
	unsigned long counter;

	fusb_func_trace("START");

	if (enable) {
		/* initialize F_USB20HDC DMA controller register */
		f_usb20hdc_initialize_dma_controller(f_usb20hdc);

		/* initialize F_USB20HDC controller */
		f_usb20hdc_initialize_controller(f_usb20hdc, 0);

		/* enable device mode */
		set_host_en(f_usb20hdc->register_base_address, 0);
		set_dev_en(f_usb20hdc->register_base_address, 1U);
		set_dev_int_mode(f_usb20hdc->register_base_address, 1U);
		set_dev_addr_load_mode(f_usb20hdc->register_base_address, 1U);

		/* enable interrupt factor */
		set_dev_inten(f_usb20hdc->register_base_address, 1U);
		set_phy_err_inten(f_usb20hdc->register_base_address, 1U);
		set_suspende_inten(f_usb20hdc->register_base_address, 1U);
		set_suspendb_inten(f_usb20hdc->register_base_address, 1U);
		set_setup_inten(f_usb20hdc->register_base_address, 1U);
		set_usbrste_inten(f_usb20hdc->register_base_address, 1U);
		set_usbrstb_inten(f_usb20hdc->register_base_address, 1U);

		/* change device state */
		f_usb20hdc_set_device_state(f_usb20hdc, USB_STATE_POWERED);

		/* initialize endpoint configure data */
		f_usb20hdc_initialize_endpoint_configure(f_usb20hdc);

		if (f_usb20hdc_pullup_flag) {
			/* pull-up D+ terminal */
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "D+ terminal is pull-up.\n");
			enable_dplus_line_pullup(f_usb20hdc->register_base_address, 1U);
		}

	} else {
		/* disable interrupt factor */
		set_dev_inten(f_usb20hdc->register_base_address, 0);
		set_phy_err_inten(f_usb20hdc->register_base_address, 0);
		set_suspende_inten(f_usb20hdc->register_base_address, 0);
		set_suspendb_inten(f_usb20hdc->register_base_address, 0);
		set_setup_inten(f_usb20hdc->register_base_address, 0);
		set_usbrste_inten(f_usb20hdc->register_base_address, 0);
		set_usbrstb_inten(f_usb20hdc->register_base_address, 0);

		/* clear interrupt factor */
		clear_phy_err_int(f_usb20hdc->register_base_address);
		clear_cmd_int(f_usb20hdc->register_base_address);
		clear_suspende_int(f_usb20hdc->register_base_address);
		clear_suspendb_int(f_usb20hdc->register_base_address);
		clear_sof_int(f_usb20hdc->register_base_address);
		clear_setup_int(f_usb20hdc->register_base_address);
		clear_usbrste_int(f_usb20hdc->register_base_address);
		clear_usbrstb_int(f_usb20hdc->register_base_address);
		clear_status_ok_int(f_usb20hdc->register_base_address);
		clear_status_ng_int(f_usb20hdc->register_base_address);

		/* change device state */
		f_usb20hdc_set_device_state(f_usb20hdc, USB_STATE_NOTATTACHED);

		/* abort previous transfer */
		f_usb20hdc_abort_in_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0], 0);
		f_usb20hdc_abort_out_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0], 0);
		f_usb20hdc_enable_endpoint(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0], 0);
		for (counter = ENDPOINT_CHANNEL_ENDPOINT1; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
			f_usb20hdc->endpoint[counter].transfer_direction ?
			f_usb20hdc_abort_in_transfer(&f_usb20hdc->endpoint[counter], 0) : f_usb20hdc_abort_out_transfer(&f_usb20hdc->endpoint[counter], 0);
			f_usb20hdc_enable_endpoint(&f_usb20hdc->endpoint[counter], 0);
		}

		/* reset F_USB20HDC DMA controller register */
		f_usb20hdc_initialize_dma_controller(f_usb20hdc);

		/* reset F_USB20HDC controller */
		f_usb20hdc_initialize_controller(f_usb20hdc, 0);

		/* dequeue all previous transfer request */
		for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++)
			f_usb20hdc_dequeue_all_transfer_request(&f_usb20hdc->endpoint[counter], -ESHUTDOWN);

		/* initialize endpoint list data */
		INIT_LIST_HEAD(&f_usb20hdc->gadget.ep0->ep_list);
		INIT_LIST_HEAD(&f_usb20hdc->gadget.ep_list);
		for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
			list_add_tail(&f_usb20hdc->endpoint[counter].endpoint.ep_list,
				      counter == ENDPOINT_CHANNEL_ENDPOINT0 ? &f_usb20hdc->gadget.ep0->ep_list : &f_usb20hdc->gadget.ep_list);
			INIT_LIST_HEAD(&f_usb20hdc->endpoint[counter].queue);
		}

		/* initialize F_USB20HDC UDC device driver structure data */
		f_usb20hdc->gadget.speed = USB_SPEED_UNKNOWN;
		f_usb20hdc->configure_value_last = 0;
		f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_SETUP;
		f_usb20hdc->control_transfer_priority_direction = 1U;

		if (f_usb20hdc_pullup_flag) {
			/* pull-down D+ terminal */
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "D+ terminal is pull-down (soft reset).\n");
			set_soft_reset(f_usb20hdc->register_base_address);
		}
	}
	fusb_func_trace("END");
	return;
}

static void f_usb20hdc_enable_communicate(struct f_usb20hdc_udc *f_usb20hdc, unsigned char enable)
{
	if (enable) {
		/* get current bus connect */
		f_usb20hdc->bus_connect = is_bus_connected(f_usb20hdc->register_base_address);

		/* check bus connect */
		if (f_usb20hdc->bus_connect) {
			/* set bus disconnect detect */
			set_bus_connect_detect(f_usb20hdc->register_base_address, 0);

			/* enable host connect */
			f_usb20hdc_enable_host_connect(f_usb20hdc, 1U);
		} else {
			/* set bus connect detect */
			set_bus_connect_detect(f_usb20hdc->register_base_address, 1U);
		}
	} else {
		/* disable host connect */
		f_usb20hdc_enable_host_connect(f_usb20hdc, 0);
	}

	return;
}

static void f_usb20hdc_on_detect_bus_connect(unsigned long data)
{
	struct f_usb20hdc_udc *f_usb20hdc;

	/* get a device driver parameter */
	f_usb20hdc = (struct f_usb20hdc_udc*)data;

	/* get current bus connect */
	f_usb20hdc->bus_connect = is_bus_connected(f_usb20hdc->register_base_address);

	/* check bus connect */
	if (f_usb20hdc->bus_connect) {
		/* set bus disconnect detect */
		set_bus_connect_detect(f_usb20hdc->register_base_address, 0);

		/* check F_USB20HDC UDC device driver register */
		if (f_usb20hdc->device_driver_register)
			/* enable host connect */
			f_usb20hdc_enable_host_connect(f_usb20hdc, 1U);
	} else {
		/* check F_USB20HDC UDC device driver register */
		if (f_usb20hdc->device_driver_register) {
			/* disable host connect */
			f_usb20hdc_enable_host_connect(f_usb20hdc, 0);

			/* set bus connect detect */
			set_bus_connect_detect(f_usb20hdc->register_base_address, 1U);

			/* notify disconnect to gadget driver */
			if ((f_usb20hdc->gadget_driver) && (f_usb20hdc->gadget_driver->disconnect))
				f_usb20hdc->gadget_driver->disconnect(&f_usb20hdc->gadget);
		} else {
			/* set bus connect detect */
			set_bus_connect_detect(f_usb20hdc->register_base_address, 1U);
		}
	}

	return;
}

static void f_usb20hdc_on_begin_bus_reset(struct f_usb20hdc_udc *f_usb20hdc)
{
	unsigned long counter;

	fusb_func_trace("START");

	/* abort previous transfer and initialize all endpoint, disable all endpoint */
	f_usb20hdc_abort_in_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0], 0);
	f_usb20hdc_abort_out_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0], 0);
	f_usb20hdc_enable_endpoint(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0], 0);

	/* re-enable endpoint 0 */
	f_usb20hdc_enable_endpoint(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0], 1U);

	for (counter = ENDPOINT_CHANNEL_ENDPOINT1; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		f_usb20hdc->endpoint[counter].transfer_direction ?
		f_usb20hdc_abort_in_transfer(&f_usb20hdc->endpoint[counter], 0) : f_usb20hdc_abort_out_transfer(&f_usb20hdc->endpoint[counter], 0);
		f_usb20hdc_enable_endpoint(&f_usb20hdc->endpoint[counter], 0);
	}

	/* initialize F_USB20HDC DMA controller register */
	f_usb20hdc_initialize_dma_controller(f_usb20hdc);

	/* dequeue all previous transfer request */
	for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++)
		f_usb20hdc_dequeue_all_transfer_request(&f_usb20hdc->endpoint[counter], -ECONNABORTED);

	/* initialize endpoint list data */
	INIT_LIST_HEAD(&f_usb20hdc->gadget.ep0->ep_list);
	INIT_LIST_HEAD(&f_usb20hdc->gadget.ep_list);
	for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		list_add_tail(&f_usb20hdc->endpoint[counter].endpoint.ep_list,
			      counter == ENDPOINT_CHANNEL_ENDPOINT0 ? &f_usb20hdc->gadget.ep0->ep_list : &f_usb20hdc->gadget.ep_list);
		INIT_LIST_HEAD(&f_usb20hdc->endpoint[counter].queue);
	}

	/* check configured */
	if (f_usb20hdc->configure_value_last) {
		/* notify dummy disconnect to gadget driver for gadget driver re-init */
		if ((f_usb20hdc->gadget_driver) && (f_usb20hdc->gadget_driver->disconnect)) {
			if (f_usb20hdc->lock_cnt > 0)
				spin_unlock(&f_usb20hdc->lock);
			f_usb20hdc->gadget_driver->disconnect(&f_usb20hdc->gadget);
			if (f_usb20hdc->lock_cnt > 0)
				spin_lock(&f_usb20hdc->lock);
		}
	}

	/* initialize F_USB20HDC UDC device driver structure data */
	f_usb20hdc->gadget.speed = USB_SPEED_UNKNOWN;
	f_usb20hdc->configure_value_last = 0;
	f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_SETUP;
	f_usb20hdc->control_transfer_priority_direction = 1U;

	fusb_func_trace("END");
	return;
}

static void f_usb20hdc_on_end_bus_reset(struct f_usb20hdc_udc *f_usb20hdc)
{
	/* set current bus speed */
	f_usb20hdc_set_bus_speed(f_usb20hdc);

	/* change device state */
	f_usb20hdc_set_device_state(f_usb20hdc, USB_STATE_DEFAULT);

	return;
}

static void f_usb20hdc_on_suspend(struct f_usb20hdc_udc *f_usb20hdc)
{
	/* check parameter */
	if (f_usb20hdc->gadget.speed == USB_SPEED_UNKNOWN)
		return;

	/* change device state */
	f_usb20hdc_set_device_state(f_usb20hdc, USB_STATE_SUSPENDED);

	/* notify suspend to gadget driver */
	if ((f_usb20hdc->gadget_driver) && (f_usb20hdc->gadget_driver->suspend)) {
		if (f_usb20hdc->lock_cnt > 0)
			spin_unlock(&f_usb20hdc->lock);
		f_usb20hdc->gadget_driver->suspend(&f_usb20hdc->gadget);
		if (f_usb20hdc->lock_cnt > 0)
			spin_lock(&f_usb20hdc->lock);
	}

	return;
}

static void f_usb20hdc_on_wakeup(struct f_usb20hdc_udc *f_usb20hdc)
{
	/* check parameter */
	if (f_usb20hdc->gadget.speed == USB_SPEED_UNKNOWN)
		return;

	/* change device state */
	f_usb20hdc_set_device_state(f_usb20hdc, f_usb20hdc->device_state_last);

	/* notify resume to gadget driver */
	if ((f_usb20hdc->gadget_driver) && (f_usb20hdc->gadget_driver->resume)) {
		if (f_usb20hdc->lock_cnt > 0)
			spin_unlock(&f_usb20hdc->lock);
		f_usb20hdc->gadget_driver->resume(&f_usb20hdc->gadget);
		if (f_usb20hdc->lock_cnt > 0)
			spin_lock(&f_usb20hdc->lock);
	}
	return;
}

static void f_usb20hdc_on_end_transfer_sof(struct f_usb20hdc_udc *f_usb20hdc)
{
	/* current non process */
	return;
}

static void f_usb20hdc_on_end_control_setup_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
#define    STANDARD_REQUEST_MAXIMUM    13U
	static unsigned char (* const standard_request_respond_function[])(struct f_usb20hdc_udc_endpoint *, struct usb_ctrlrequest) = {
		f_usb20hdc_respond_standard_request_get_status,
		f_usb20hdc_respond_standard_request_clear_feature,
		f_usb20hdc_respond_standard_request_undefined,
		f_usb20hdc_respond_standard_request_set_feature,
		f_usb20hdc_respond_standard_request_undefined,
		f_usb20hdc_respond_standard_request_set_address,
		f_usb20hdc_respond_standard_request_get_descriptor,
		f_usb20hdc_respond_standard_request_set_descriptor,
		f_usb20hdc_respond_standard_request_get_configuration,
		f_usb20hdc_respond_standard_request_set_configuration,
		f_usb20hdc_respond_standard_request_get_interface,
		f_usb20hdc_respond_standard_request_set_interface,
		f_usb20hdc_respond_standard_request_sync_frame,
	} ;
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;
	struct usb_ctrlrequest ctrlrequest;
	unsigned long bytes;

	/* get SETUP transfer byte */
	bytes = get_phycnt(f_usb20hdc->register_base_address, 1U);

	/* check SETUP transfer byte */
	if (bytes != 8U) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():SETUP tranfer byte is mismatch at bytes = %u.\n",
			__FUNCTION__, (unsigned int)bytes);
		/* check new setup transfer */
		if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
			/* protocol stall */
			f_usb20hdc_halt_transfer(endpoint, 1U, 0);
		return;
	}

	/* clear status stage delay */
	f_usb20hdc->control_transfer_status_stage_delay = 0;

	/* clear transfer halt */
	endpoint->halt = 0;
	endpoint->force_halt = 0;
	endpoint->null_packet_transfer = 0;

	/* dequeue all previous control transfer request */
	f_usb20hdc_dequeue_all_transfer_request(endpoint, -EPROTO);

	/* read SETUP transfer data form buffer */
	read_epbuf(f_usb20hdc->register_base_address, endpoint->buffer_address_offset[1U], (unsigned long *)&ctrlrequest, bytes);

	/* check new setup transfer */
	if (f_usb20hdc_is_setup_transferred(f_usb20hdc))
		return;

	/* enable next OUT transfer */
	set_bufrd(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

	/* disable readyi / readyo interrupt */
	set_readyi_ready_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);
	set_readyo_empty_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 0);

	/* update control transfer stage */
	if (ctrlrequest.bRequestType & USB_DIR_IN) {
		f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_IN_DATA;
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is IN data stage.\n");
	} else {
		if (ctrlrequest.wLength) {
			f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_OUT_DATA;
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is OUT data stage.\n");
		} else {
			f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_IN_STATUS;
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is IN status stage.\n");
		}
	}

	/* check request type */
	switch (ctrlrequest.bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
		/* process standard request respond */
		if ((ctrlrequest.bRequest >= STANDARD_REQUEST_MAXIMUM) ||
		    (!standard_request_respond_function[ctrlrequest.bRequest](endpoint, ctrlrequest))) {
			if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
				/* protocol stall */
				f_usb20hdc_halt_transfer(endpoint, 1U, 0);
		}

		/* check NULL packet IN transfer for status stage delay */
		if (f_usb20hdc->control_transfer_status_stage_delay)
			return;
		break;
	case USB_TYPE_CLASS:
		/* process class request respond */
		if (!f_usb20hdc_respond_class_request(endpoint, ctrlrequest)) {
			if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
				/* protocol stall */
				f_usb20hdc_halt_transfer(endpoint, 1U, 0);
		}

		/* check NULL packet IN transfer for status stage delay */
		if (f_usb20hdc->control_transfer_status_stage_delay)
			return;
		break;
	case USB_TYPE_VENDOR:
		/* process vendor request respond */
		if (!f_usb20hdc_respond_vendor_request(endpoint, ctrlrequest)) {
			if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
				/* protocol stall */
				f_usb20hdc_halt_transfer(endpoint, 1U, 0);
		}

		/* check NULL packet IN transfer for status stage delay */
		if (f_usb20hdc->control_transfer_status_stage_delay)
			return;
		break;
	default:
		if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
			/* protocol stall */
			f_usb20hdc_halt_transfer(endpoint, 1U, 0);
		break;
	}

	if (f_usb20hdc_is_setup_transferred(f_usb20hdc))
		return;

	/* enable status stage transfer */
	if (ctrlrequest.bRequestType & USB_DIR_IN) {
		f_usb20hdc->control_transfer_priority_direction = 1U;
		set_readyo_empty_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 1U);
	} else {
		f_usb20hdc->control_transfer_priority_direction = ctrlrequest.wLength ? 0 : 1U;
		set_appcnt(f_usb20hdc->register_base_address, 0, 0);
		set_bufwr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		set_readyi_ready_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0, 1U);
	}

	return;
}

static void f_usb20hdc_on_end_control_in_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;

	/* process control transfer stage */
	switch (f_usb20hdc->control_transfer_stage) {
	case CONTROL_TRANSFER_STAGE_IN_DATA:
		/* check new SETUP transfer */
		if (f_usb20hdc_is_setup_transferred(f_usb20hdc))
			break;

		/* check IN transfer continue */
		if (!f_usb20hdc_end_in_transfer_pio(endpoint))
			break;

		/* update control transfer stage */
		f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_OUT_STATUS;
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is OUT status stage.\n");
		break;
	case CONTROL_TRANSFER_STAGE_OUT_DATA:
	case CONTROL_TRANSFER_STAGE_IN_STATUS:
		/* update control transfer stage */
		f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_SETUP;
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is SETUP stage.\n");

		/* notify request complete */
		f_usb20hdc_notify_transfer_request_complete(endpoint, 0);
		break;
	case CONTROL_TRANSFER_STAGE_OUT_STATUS:
		/* non process */
		break;
	default:
		/* check new SETUP transfer */
		if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
			/* protocol stall */
			f_usb20hdc_halt_transfer(endpoint, 1U, 0);

		/* update control transfer stage */
		f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_SETUP;
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is SETUP stage.\n");
		break;
	}

	return;
}

static void f_usb20hdc_on_end_control_out_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc = endpoint->f_usb20hdc;

	/* process control transfer stage */
	switch (f_usb20hdc->control_transfer_stage) {
	case CONTROL_TRANSFER_STAGE_OUT_DATA:
		/* check new SETUP transfer */
		if (f_usb20hdc_is_setup_transferred(f_usb20hdc))
			break;

		/* check OUT transfer continue */
		if (!f_usb20hdc_end_out_transfer_pio(endpoint))
			break;

		/* update control transfer stage */
		f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_IN_STATUS;
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is IN status stage.\n");
		break;
	case CONTROL_TRANSFER_STAGE_IN_DATA:
	case CONTROL_TRANSFER_STAGE_OUT_STATUS:
		/* update control transfer stage */
		f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_SETUP;
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is SETUP stage.\n");

		/* notify request complete */
		f_usb20hdc_notify_transfer_request_complete(endpoint, 0);
		break;
	default:
		/* check new SETUP transfer */
		if (!f_usb20hdc_is_setup_transferred(f_usb20hdc))
			/* protocol stall */
			f_usb20hdc_halt_transfer(endpoint, 1U, 0);

		/* update control transfer stage */
		f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_SETUP;
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "next control transfer stage is SETUP stage.\n");
		break;
	}

	return;
}

static void f_usb20hdc_on_end_in_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_request *request;
	unsigned char endpoint_channel;

	fusb_func_trace("START");

	f_usb20hdc = endpoint->f_usb20hdc;
	endpoint_channel = endpoint->endpoint_channel;

	/* check IN transfer continue */
	if (!f_usb20hdc_end_in_transfer(endpoint))
		/* to be continued */
	{
		fusb_func_trace("END 1");
		return;
	}

	/* notify request complete */
	f_usb20hdc_notify_transfer_request_complete(endpoint, 0);

	/* check next queue empty */
	if (list_empty(&endpoint->queue)) {
		fusb_func_trace("END 2");
		return;
	}

	/* get next request */
	request = list_entry(endpoint->queue.next, struct f_usb20hdc_udc_request, queue);
	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u is next queue at request = 0x%p, length = %u, buf = 0x%p.\n",
		endpoint_channel, request, request->request.length, request->request.buf);
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u halt status is %u.\n",endpoint_channel, endpoint->halt);

	/* check the got next request a request under current execution */
	if (request->request_execute) {
		fusb_func_trace("END 3");
		return;
	}

	/* save request */
	endpoint->request = request;

	/* set endpoint x IN trasfer request */
	if (!f_usb20hdc_set_in_transfer(endpoint)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():f_usb20hdc_set_in_transfer() of endpoint %u is failed.\n",
			__FUNCTION__, endpoint_channel);
		f_usb20hdc_dequeue_all_transfer_request(endpoint, -EPERM);
	}

	fusb_func_trace("END 4");
	return;
}

static void f_usb20hdc_on_end_out_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_request *request;
	unsigned char endpoint_channel;

	f_usb20hdc = endpoint->f_usb20hdc;
	endpoint_channel = endpoint->endpoint_channel;

	/* check OUT transfer continue */
	if (!f_usb20hdc_end_out_transfer(endpoint))
		/* to be continued */
		return;

	/* notify request complete */
	f_usb20hdc_notify_transfer_request_complete(endpoint, 0);

	/* check next queue empty */
	if (list_empty(&endpoint->queue))
		return;

	/* get next request */
	request = list_entry(endpoint->queue.next, struct f_usb20hdc_udc_request, queue);
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u is next queue at request = 0x%p, length = %u, buf = 0x%p.\n",
		endpoint_channel, request, request->request.length, request->request.buf);
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u halt status is %u.\n",endpoint_channel, endpoint->halt);

	/* check the got next request a request under current execution */
	if (request->request_execute)
		return;

	/* save request */
	endpoint->request = request;

	/* set endpoint x OUT transfer request */
	if (!f_usb20hdc_set_out_transfer(endpoint)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():f_usb20hdc_set_out_transfer() of endpoint %u is failed.\n",
			__FUNCTION__, endpoint_channel);
		f_usb20hdc_dequeue_all_transfer_request(endpoint, -EPERM);
	}

	return;
}

static void f_usb20hdc_on_halt_transfer(struct f_usb20hdc_udc_endpoint *endpoint)
{
	/* current non process */
	return;
}

static void f_usb20hdc_on_recovery_controller_hungup(struct f_usb20hdc_udc *f_usb20hdc)
{
	return;
}

static irqreturn_t f_usb20hdc_on_usb_function(int irq, void *dev_id)
{
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc = dev_id;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!dev_id))
		return IRQ_NONE;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* check F_USB20HDC controller interrupt request assert */
	if (unlikely(irq != f_usb20hdc->irq)) {
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended at non process.\n", __FUNCTION__);
		return IRQ_NONE;
	}

	f_usb_spin_lock(f_usb20hdc);

	/* PHY hung-up interrupt factor */
	if ((get_phy_err_int(f_usb20hdc->register_base_address)) &&
	    (get_phy_err_inten(f_usb20hdc->register_base_address))) {
		/* clear phy_err interrupt factor */
		clear_phy_err_int(f_usb20hdc->register_base_address);

		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():phy_err interrrupt is occurred.\n", __FUNCTION__);

		/* process recovery controller hung-up */
		f_usb20hdc_on_recovery_controller_hungup(f_usb20hdc);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

		f_usb_spin_unlock(f_usb20hdc);
		fusb_func_trace("END PHY hung-up interrupt");
		return IRQ_HANDLED;
	}

	/* suspend begin interrupt factor */
	if ((get_suspendb_int(f_usb20hdc->register_base_address)) &&
	    (get_suspendb_inten(f_usb20hdc->register_base_address))) {
		/* clear suspendb interrupt factor */
		clear_suspendb_int(f_usb20hdc->register_base_address);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "suspendb interrrupt is occurred.\n");

		/* process bus suspend */
		f_usb20hdc_on_suspend(f_usb20hdc);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		f_usb_spin_unlock(f_usb20hdc);
		fusb_func_trace("END suspend begin interrupt");
		return IRQ_HANDLED;
	}

	/* suspend end interrupt factor */
	if ((get_suspende_int(f_usb20hdc->register_base_address)) &&
	    (get_suspende_int(f_usb20hdc->register_base_address))) {
		/* clear suspende interrupt factor */
		clear_suspende_int(f_usb20hdc->register_base_address);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "suspende interrrupt is occurred.\n");

		/* process bus wakeup */
		f_usb20hdc_on_wakeup(f_usb20hdc);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		f_usb_spin_unlock(f_usb20hdc);
		fusb_func_trace("END suspend end interrupt");
		return IRQ_HANDLED;
	}

	/* bus reset begin interrupt factor */
	if ((get_usbrstb_int(f_usb20hdc->register_base_address)) &&
	    (get_usbrstb_inten(f_usb20hdc->register_base_address))) {
		/* clear usbrstb interrupt factor */
		clear_usbrstb_int(f_usb20hdc->register_base_address);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "usbrstb interrrupt is occurred.\n");

		/* process bus reset begin */
		f_usb20hdc_on_begin_bus_reset(f_usb20hdc);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		f_usb_spin_unlock(f_usb20hdc);
		fusb_func_trace("END bus reset begin interrupt");
		return IRQ_HANDLED;
	}

	/* bus reset end interrupt factor */
	if ((get_usbrste_int(f_usb20hdc->register_base_address)) &&
	    (get_usbrste_inten(f_usb20hdc->register_base_address))) {
		/* clear usbrste interrupt factor */
		clear_usbrste_int(f_usb20hdc->register_base_address);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "usbrste interrrupt is occurred.\n");

		/* process bus reset end */
		f_usb20hdc_on_end_bus_reset(f_usb20hdc);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		f_usb_spin_unlock(f_usb20hdc);
		fusb_func_trace("END bus reset end interrupt");
		return IRQ_HANDLED;
	}

	/* SOF interrupt factor */
	if ((get_sof_int(f_usb20hdc->register_base_address)) &&
	    (get_sof_inten(f_usb20hdc->register_base_address))) {
		/* clear sof interrupt factor */
		clear_sof_int(f_usb20hdc->register_base_address);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "sof interrrupt is occurred.\n");

		/* process SOF transfer end */
		f_usb20hdc_on_end_transfer_sof(f_usb20hdc);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		f_usb_spin_unlock(f_usb20hdc);
		fusb_func_trace("END SOF interrupt");
		return IRQ_HANDLED;
	}

	/* SETUP transfer interrupt factor */
	if ((get_setup_int(f_usb20hdc->register_base_address)) &&
	    (get_setup_inten(f_usb20hdc->register_base_address))) {
		/* clear setup interrupt factor */
		clear_setup_int(f_usb20hdc->register_base_address);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "setup interrrupt is occurred.\n");

		/* clear readyi / ready0 interrupt factor */
		clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		clear_readyo_empty_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

		/* process control SETUP transfer end */
		f_usb20hdc_on_end_control_setup_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0]);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		f_usb_spin_unlock(f_usb20hdc);
		fusb_func_trace("END SETUP transfer interrupt");
		return IRQ_HANDLED;
	}

	/* endpoint x dma interrupt factor */
	for (counter = USB_DMA_CHANNEL_DMA0; counter < F_USB20HDC_UDC_CONFIG_USB_DMA_CHANNELS; counter++) {
		if ((get_dma_int(f_usb20hdc->register_base_address, counter)) &&
		    (get_dma_inten(f_usb20hdc->register_base_address, counter))) {
			/* clear dma interrupt factor */
			clear_dma_int(f_usb20hdc->register_base_address, counter);

			hdc_dbg(f_usb20hdc->gadget.dev.parent, "dma %u interrrupt is occurred.\n", (unsigned int)counter);

			/* process IN / OUT transfer end */
			f_usb20hdc->endpoint[f_usb20hdc->usb_dma_endpoint_channel[counter]].transfer_direction ?
			f_usb20hdc_on_end_in_transfer(&f_usb20hdc->endpoint[f_usb20hdc->usb_dma_endpoint_channel[counter]]) :
			f_usb20hdc_on_end_out_transfer(&f_usb20hdc->endpoint[f_usb20hdc->usb_dma_endpoint_channel[counter]]);
			f_usb_spin_unlock(f_usb20hdc);
			fusb_func_trace("END endpoint x dma interrupt");
			return IRQ_HANDLED;
		}
	}

	/* endpoint x interrupt factor */
	for (counter = ENDPOINT_CHANNEL_ENDPOINT1; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		if ((get_dev_ep_int(f_usb20hdc->register_base_address, counter)) &&
		    (get_dev_ep_inten(f_usb20hdc->register_base_address, counter))) {

			hdc_dbg(f_usb20hdc->gadget.dev.parent, "dev_ep endpoint %u interrrupt is occurred.\n", (unsigned int)counter);

			if ((get_readyi_ready_int(f_usb20hdc->register_base_address, counter)) &&
			    (get_readyi_ready_inten(f_usb20hdc->register_base_address, counter))) {
				/* clear ready interrupt factor */
				clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, counter);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "ready interrrupt is occurred.\n");

				/* process IN / OUT transfer end */
				f_usb20hdc->endpoint[counter].transfer_direction ?
				f_usb20hdc_on_end_in_transfer(&f_usb20hdc->endpoint[counter]) : f_usb20hdc_on_end_out_transfer(&f_usb20hdc->endpoint[counter]);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				f_usb_spin_unlock(f_usb20hdc);
				fusb_func_trace("END ready interrupt factor");
				return IRQ_HANDLED;
			}

			if ((get_readyo_empty_int(f_usb20hdc->register_base_address, counter)) &&
			    (get_readyo_empty_inten(f_usb20hdc->register_base_address, counter))) {
				/* clear empty interrupt factor */
				clear_readyo_empty_int_clr(f_usb20hdc->register_base_address, counter);
				hdc_dbg(f_usb20hdc->gadget.dev.parent, "empty interrrupt is occurred.\n");

				/* process IN transfer end */
				if (f_usb20hdc->endpoint[counter].transfer_direction)
					f_usb20hdc_on_end_in_transfer(&f_usb20hdc->endpoint[counter]);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				f_usb_spin_unlock(f_usb20hdc);
				fusb_func_trace("END empty interrupt factor");
				return IRQ_HANDLED;
			}

			if ((get_stalled_int(f_usb20hdc->register_base_address, counter)) &&
			    (get_stalled_inten(f_usb20hdc->register_base_address, counter))) {
				/* clear stalled interrupt factor */
				clear_stalled_int_clr(f_usb20hdc->register_base_address, counter);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "stalled interrrupt is occurred.\n");

				/* process transfer halt */
				f_usb20hdc_on_halt_transfer(&f_usb20hdc->endpoint[counter]);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				f_usb_spin_unlock(f_usb20hdc);
				fusb_func_trace("END stalled interrupt factor");
				return IRQ_HANDLED;
			}

			hdc_dbg(f_usb20hdc->gadget.dev.parent, "other interrrupt is occurred.\n");

			/* clear endpoint x interrupt */
			clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, counter);
			clear_readyo_empty_int_clr(f_usb20hdc->register_base_address, counter);
			clear_ping_int_clr(f_usb20hdc->register_base_address, counter);
			clear_stalled_int_clr(f_usb20hdc->register_base_address, counter);
			clear_nack_int_clr(f_usb20hdc->register_base_address, counter);

			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
			f_usb_spin_unlock(f_usb20hdc);
			fusb_func_trace("END endpoint x interrupt");
			return IRQ_HANDLED;
		}
	}

	/* endpoint 0 interrupt factor */
	if ((get_dev_ep_int(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0)) &&
	    (get_dev_ep_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0))) {

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "dev_ep endpoint 0 interrrupt is occurred.\n");

		/* check priority direction */
		if (f_usb20hdc->control_transfer_priority_direction) {
			/* priority is given to IN transfer */
			if ((get_readyi_ready_int(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0)) &&
			    (get_readyi_ready_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0))) {
				/* clear readyi interrupt factor */
				clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "readyi interrrupt is occurred.\n");

				/* process control IN transfer end */
				f_usb20hdc_on_end_control_in_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0]);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				f_usb_spin_unlock(f_usb20hdc);
				fusb_func_trace("END IN transfer readyi interrupt");
				return IRQ_HANDLED;
			}

			if ((get_readyo_empty_int(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0)) &&
			    (get_readyo_empty_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0))) {
				/* clear readyo interrupt factor */
				clear_readyo_empty_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "readyo interrrupt is occurred.\n");

				/* process control OUT transfer end */
				f_usb20hdc_on_end_control_out_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0]);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				f_usb_spin_unlock(f_usb20hdc);
				fusb_func_trace("END IN transfer readyo interrupt");
				return IRQ_HANDLED;
			}
		} else {
			/* priority is given to OUT transfer */
			if ((get_readyo_empty_int(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0)) &&
			    (get_readyo_empty_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0))) {
				/* clear readyo interrupt factor */
				clear_readyo_empty_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "readyo interrrupt is occurred.\n");

				/* process control OUT transfer end */
				f_usb20hdc_on_end_control_out_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0]);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				f_usb_spin_unlock(f_usb20hdc);
				fusb_func_trace("END OUT transfer readyo interrup");
				return IRQ_HANDLED;
			}

			if ((get_readyi_ready_int(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0)) &&
			    (get_readyi_ready_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0))) {
				/* clear readyi interrupt factor */
				clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "readyi interrrupt is occurred.\n");

				/* process control IN transfer end */
				f_usb20hdc_on_end_control_in_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0]);

				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				f_usb_spin_unlock(f_usb20hdc);
				fusb_func_trace("END OUT transfer readyi interrupt");
				return IRQ_HANDLED;
			}
		}

		if ((get_stalled_int(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0)) &&
		    (get_stalled_inten(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0))) {
			/* clear stalled interrupt factor */
			clear_stalled_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

			hdc_dbg(f_usb20hdc->gadget.dev.parent, "stalled 0 interrrupt is occurred.\n");

			/* process transfer halt */
			f_usb20hdc_on_halt_transfer(&f_usb20hdc->endpoint[ENDPOINT_CHANNEL_ENDPOINT0]);

			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
			f_usb_spin_unlock(f_usb20hdc);
			fusb_func_trace("END clear stalled interrupt");
			return IRQ_HANDLED;
		}

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "other interrrupt is occurred.\n");

		/* clear endpoint 0 interrupt */
		clear_readyo_empty_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		clear_readyi_ready_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		clear_ping_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		clear_stalled_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);
		clear_nack_int_clr(f_usb20hdc->register_base_address, ENDPOINT_CHANNEL_ENDPOINT0);

		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		f_usb_spin_unlock(f_usb20hdc);
		fusb_func_trace("END endpoint 0 interrupt");
		return IRQ_HANDLED;
	}

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
	f_usb_spin_unlock(f_usb20hdc);
	fusb_func_trace("END");

	return IRQ_HANDLED;
}

/* So far, it is implementation of F_USB20HDC UDC device driver local function            */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HDC UDC device driver external function            */

int usb_gadget_probe_driver(struct usb_gadget_driver *driver,
			     int (*bind)(struct usb_gadget *))
{
	struct f_usb20hdc_udc *f_usb20hdc = f_usb20hdc_data;
	int result;

	fusb_func_trace("START");

	/* check data */
	if (unlikely(!f_usb20hdc))
		return -ENODEV;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* check argument */
	if (unlikely(!driver)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():driver is null pointer.\n", __FUNCTION__);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}
	if (unlikely((driver->speed != USB_SPEED_HIGH) && (driver->speed != USB_SPEED_FULL))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():speed of driver is error.\n", __FUNCTION__);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* check parameter */
	if (unlikely(f_usb20hdc->gadget_driver)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():F_USB20HDC UDC driver is busy.\n", __FUNCTION__);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EBUSY;
	}

	/* entry gadget driver structure */
	f_usb20hdc->gadget_driver = driver;
	f_usb20hdc->gadget.dev.driver = &driver->driver;

	/* initialize endpoint configure data */
	f_usb20hdc_initialize_endpoint_configure(f_usb20hdc);

	/* add device */
	result = device_add(&f_usb20hdc->gadget.dev);
	if (result) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():device_add() is failed at %d.\n", __FUNCTION__, result);
		f_usb20hdc->gadget_driver = NULL;
		f_usb20hdc->gadget.dev.driver = NULL;
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return result;
	}

	/* notify bind to gadget driver */
	if (bind) {
		result = bind(&f_usb20hdc->gadget);

		if (result) {
			hdc_err(f_usb20hdc->gadget.dev.parent, "%s():%s of bind() is failed at %d.\n",
				__FUNCTION__, f_usb20hdc->gadget_driver->driver.name, result);
			device_del(&f_usb20hdc->gadget.dev);
			f_usb20hdc->gadget_driver = NULL;
			f_usb20hdc->gadget.dev.driver = NULL;
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
			return result;
		}
	}

	/* initialize F_USB20HDC UDC device driver structure data */
	f_usb20hdc->gadget.speed = USB_SPEED_UNKNOWN;
	f_usb20hdc->highspeed_support = (f_usb20hdc->gadget_driver->speed == USB_SPEED_HIGH) && (gadget_is_dualspeed(&f_usb20hdc->gadget)) ? 1U : 0;
	f_usb20hdc->bus_connect = 0;
	f_usb20hdc->device_state = USB_STATE_ATTACHED;
	f_usb20hdc->device_state_last = USB_STATE_NOTATTACHED;
	f_usb20hdc->configure_value_last = 0;
	f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_SETUP;
	f_usb20hdc->control_transfer_priority_direction = 1U;

	/* set F_USB20HDC UDC device driver register */
	f_usb20hdc->device_driver_register = 1U;

	/* enable communicate */
	f_usb20hdc_enable_communicate(f_usb20hdc, 1U);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s is registered.\n", f_usb20hdc->gadget_driver->driver.name);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}
EXPORT_SYMBOL(usb_gadget_probe_driver);

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct f_usb20hdc_udc *f_usb20hdc = f_usb20hdc_data;
	unsigned char device_driver_register_last;
	unsigned long flags;

	fusb_func_trace("START");

	/* check data */
	if (unlikely(!f_usb20hdc))
		return -ENODEV;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check argument */
	if (unlikely(!driver)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():driver pointer is error.\n", __FUNCTION__);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}
	if (unlikely(driver != f_usb20hdc->gadget_driver)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():driver is mismatch.\n", __FUNCTION__);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* clear F_USB20HDC UDC device driver register */
	device_driver_register_last = f_usb20hdc->device_driver_register;
	f_usb20hdc->device_driver_register = 0;

	/* disable communicate */
	f_usb20hdc_enable_communicate(f_usb20hdc, 0);

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	/* notify disconnect to gadget driver */
	if (f_usb20hdc->gadget_driver->disconnect)
		f_usb20hdc->gadget_driver->disconnect(&f_usb20hdc->gadget);

	/* notify unbind to gadget driver */
	if (f_usb20hdc->gadget_driver->unbind)
		f_usb20hdc->gadget_driver->unbind(&f_usb20hdc->gadget);

	f_usb20hdc->gadget_driver = NULL;

	/* delete device */
	if (device_driver_register_last)
		device_del(&f_usb20hdc->gadget.dev);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s is unregistered.\n", f_usb20hdc->gadget_driver->driver.name);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}
EXPORT_SYMBOL(usb_gadget_unregister_driver);

/* So far, it is implementation of F_USB20HDC UDC device driver external function        */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HDC UDC device driver upper layer interface            */

static int f_usb20hdc_udc_ep_enable(struct usb_ep *ep, const struct usb_endpoint_descriptor *desc)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely((!ep) || (!desc)))
		return -EINVAL;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check parameter */
	if (unlikely(endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint 0 enable is invalid.\n", __FUNCTION__);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* check endpoint descriptor parameter */
	if (unlikely(desc->bDescriptorType != USB_DT_ENDPOINT)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u descriptor type is error.\n",
			__FUNCTION__, endpoint_channel);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* check gadget driver parameter */
	if (unlikely(!f_usb20hdc->gadget_driver)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():gadget driver parameter is none.\n", __FUNCTION__);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "end point %u is enabled.\n", endpoint_channel);

	/* set endpoint parameter */
	endpoint->endpoint_descriptor = desc;

	/* configure endpoint */
	if (!f_usb20hdc_configure_endpoint(endpoint)) {
		endpoint->endpoint_descriptor = NULL;
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u configure is failed.\n",
			__FUNCTION__, endpoint_channel);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	/* dequeue all previous transfer request */
	f_usb20hdc_dequeue_all_transfer_request(endpoint, -ECONNABORTED);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* enable endpoint */
	f_usb20hdc_enable_endpoint(endpoint, 1U);

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_ep_disable(struct usb_ep *ep)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!ep))
		return -EINVAL;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check parameter */
	if (unlikely(endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint 0 disable is invalid.\n", __FUNCTION__);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "end point %u is disabled.\n", endpoint_channel);

	/* set endpoint parameter */
	endpoint->endpoint_descriptor = NULL;

	/* disable endpoint */
	f_usb20hdc_enable_endpoint(endpoint, 0);

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	/* dequeue all previous transfer request */
	f_usb20hdc_dequeue_all_transfer_request(endpoint, -ESHUTDOWN);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}

static struct usb_request *f_usb20hdc_udc_ep_alloc_request(struct usb_ep *ep, gfp_t gfp_flags) {
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_request *request;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;

	fusb_func_trace("START");

	/*
	 * [notice]:Acquisition of a spin lock is prohibition.
	 */

	/* check argument */
	if (unlikely(!ep))
		return 0;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* allocate memory and zero clear memory */
	request = kzalloc(sizeof(*request), gfp_flags);
	if (!request) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():kzalloc is failed.\n", __FUNCTION__);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return 0;
	}
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u allocate memory is 0x%p.\n", endpoint_channel, request);

	/* initialize list data */
	INIT_LIST_HEAD(&request->queue);

	request->request.dma = ~(dma_addr_t)0;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return &request->request;
}

static void f_usb20hdc_udc_ep_free_request(struct usb_ep *ep, struct usb_request *req)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_request *request;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;

	fusb_func_trace("START");

	/*
	 * [notice]:Acquisition of a spin lock is prohibition.
	 */

	/* check argument */
	if (unlikely((!ep) || (!req)))
		return;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	request = container_of(req, struct f_usb20hdc_udc_request, request);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* free memory */
	WARN_ON(!list_empty(&request->queue));
	kfree(request);
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u free memory is 0x%p.\n", endpoint_channel, request);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return;
}

static int f_usb20hdc_udc_ep_queue(struct usb_ep *ep, struct usb_request *req, gfp_t gfp_flags)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_request *request;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely((!ep) || (!req)))
		return -EINVAL;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	request = container_of(req, struct f_usb20hdc_udc_request, request);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check parameter */
	if (unlikely((!req->buf) || (!list_empty(&request->queue)))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():request parameter is error.\n", __FUNCTION__);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		req->status = -EINVAL;
		req->actual = 0;
		if (req->complete)
			req->complete(ep, req);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}
	if (unlikely((endpoint_channel != ENDPOINT_CHANNEL_ENDPOINT0) && (!endpoint->endpoint_descriptor))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u descriptor is none.\n",
			__FUNCTION__, endpoint_channel);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		req->status = -EINVAL;
		req->actual = 0;
		if (req->complete)
			req->complete(ep, req);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* check gadget driver parameter */
	if (unlikely(!f_usb20hdc->gadget_driver)) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():gadget driver parameter is none.\n", __FUNCTION__);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		req->status = -ESHUTDOWN;
		req->actual = 0;
		if (req->complete)
			req->complete(ep, req);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -ESHUTDOWN;
	}

	/* check state */
	if (f_usb20hdc->gadget.speed == USB_SPEED_UNKNOWN) {
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "device state is reset.\n");
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		req->status = -ESHUTDOWN;
		req->actual = 0;
		if (req->complete)
			req->complete(ep, req);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -ESHUTDOWN;
	}

	hdc_dbg(f_usb20hdc->gadget.dev.parent,
		"endpoint %u is queue at request = 0x%p, length = %u, buf = 0x%p.\n",
		endpoint_channel, request, request->request.length, request->request.buf);
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u halt status is %u.\n",endpoint_channel, endpoint->halt);

	/* initialize request parameter */
	request->request.status = -EINPROGRESS;
	request->request.actual = 0;

	/* check current queue execute */
	if ((!list_empty(&endpoint->queue)) || (endpoint->halt)) {
		/* add list tail */
		list_add_tail(&request->queue, &endpoint->queue);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		fusb_func_trace("END not set endpoint request");
		return 0;
	}

	/* save request */
	endpoint->request = request;

	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
		/* request endpoint 0 */
		switch (f_usb20hdc->control_transfer_stage) {
		case CONTROL_TRANSFER_STAGE_IN_DATA:
		case CONTROL_TRANSFER_STAGE_IN_STATUS:
			if (!f_usb20hdc_set_in_transfer_pio(endpoint)) {
				hdc_err(f_usb20hdc->gadget.dev.parent, "%s():f_usb20hdc_set_in_transfer_pio() of endpoint 0 is failed.\n",
					__FUNCTION__);
				f_usb20hdc_notify_transfer_request_complete(endpoint, -EPERM);
				f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				return -EPERM;
			}
			break;
		case CONTROL_TRANSFER_STAGE_OUT_DATA:
		case CONTROL_TRANSFER_STAGE_OUT_STATUS:
			if (!f_usb20hdc_set_out_transfer_pio(endpoint)) {
				hdc_err(f_usb20hdc->gadget.dev.parent, "%s():f_usb20hdc_set_out_transfer_pio() of endpoint 0 is failed.\n",
					__FUNCTION__);
				f_usb20hdc_notify_transfer_request_complete(endpoint, -EPERM);
				f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				return -EPERM;
			}
			break;
		default:
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "control transfer stage is changed at %u.\n",
				f_usb20hdc->control_transfer_stage);
			f_usb20hdc_notify_transfer_request_complete(endpoint, -EPERM);
			f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
			return -EPERM;
		}
	} else {
		/* request endpoint x */
		if (endpoint->transfer_direction) {
			if (!f_usb20hdc_set_in_transfer(endpoint)) {
				hdc_err(f_usb20hdc->gadget.dev.parent, "%s():f_usb20hdc_set_in_transfer() of endpoint %u is failed.\n",
					__FUNCTION__, endpoint_channel);
				f_usb20hdc_notify_transfer_request_complete(endpoint, -EPERM);
				f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				return -EPERM;
			}
		} else {
			if (!f_usb20hdc_set_out_transfer(endpoint)) {
				hdc_err(f_usb20hdc->gadget.dev.parent, "%s():f_usb20hdc_set_out_transfer() of endpoint %u is failed.\n",
					__FUNCTION__, endpoint_channel);
				f_usb20hdc_notify_transfer_request_complete(endpoint, -EPERM);
				f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				return -EPERM;
			}
		}
	}

	/* add list tail */
	list_add_tail(&request->queue, &endpoint->queue);

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");
	return 0;
}

static int f_usb20hdc_udc_ep_dequeue(struct usb_ep *ep, struct usb_request *req)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_request *request;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely((!ep) || (!req)))
		return -EINVAL;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	request = container_of(req, struct f_usb20hdc_udc_request, request);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check parameter */
	if (unlikely((endpoint_channel != ENDPOINT_CHANNEL_ENDPOINT0) && (!endpoint->endpoint_descriptor))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u descriptor is none.\n", __FUNCTION__, endpoint_channel);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/*
	 * check queue empty
	 * [notice]:It is mandatory processing.
	 */
	if (!list_empty(&endpoint->queue)) {
		/* check list entry */
		list_for_each_entry(request, &endpoint->queue, queue) {
			if ((request) && (&request->request == req))
				break;
		}

		/* check dequeue request mismatch */
		if (unlikely((!request) || (&request->request != req))) {
			hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u request is mismatch.\n",
				__FUNCTION__, endpoint_channel);
			f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
			return -ENOENT;
		}
	}

	/* abort request transfer */
	endpoint->request = request;
	if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
		f_usb20hdc_abort_in_transfer(endpoint, 0);
		f_usb20hdc_abort_out_transfer(endpoint, 0);
	} else {
		endpoint->transfer_direction ? f_usb20hdc_abort_in_transfer(endpoint, 0) : f_usb20hdc_abort_out_transfer(endpoint, 0);
	}
	f_usb20hdc_notify_transfer_request_complete(endpoint, -ECONNRESET);

	/* check next queue empty */
	if (list_empty(&endpoint->queue)) {
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return 0;
	}

	/* get next request */
	request = list_entry(endpoint->queue.next, struct f_usb20hdc_udc_request, queue);

	/* check the got next request a request under current execution */
	if (request->request_execute) {
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return 0;
	}

	/* save request */
	endpoint->request = request;

	/* set endpoint x transfer request */
	if (!(endpoint->transfer_direction ? f_usb20hdc_set_in_transfer(endpoint) : f_usb20hdc_set_out_transfer(endpoint))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():%s of endpoint %u is failed.\n",
			endpoint->transfer_direction ? "f_usb20hdc_set_in_transfer()" : "f_usb20hdc_set_out_transfer()", __FUNCTION__, endpoint_channel);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		f_usb20hdc_dequeue_all_transfer_request(endpoint, -EPERM);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return 0;
	}

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_ep_set_halt(struct usb_ep *ep, int value)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely((!ep) || ((value != 0) && (value != 1U))))
		return -EINVAL;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check parameter */
	if (unlikely((endpoint_channel != ENDPOINT_CHANNEL_ENDPOINT0) && (!endpoint->endpoint_descriptor))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u descriptor is none.\n",
			__FUNCTION__, endpoint_channel);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	if (value) {
		/* check current queue execute */
		if (!list_empty(&endpoint->queue)) {
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u is execute queue.\n", endpoint_channel);
			f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
			return -EPERM;
		}

		if (endpoint_channel == ENDPOINT_CHANNEL_ENDPOINT0) {
			/* check new SETUP transfer */
			if (f_usb20hdc_is_setup_transferred(f_usb20hdc)) {
				f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
				hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
				return -EPERM;
			}
		}

		/* set halt */
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u transfer is halted.\n", endpoint_channel);
		f_usb20hdc_halt_transfer(endpoint, 1U, 0);
	} else {
		/* clear halt */
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u transfer halt is cleared.\n", endpoint_channel);

		f_usb20hdc_halt_transfer(endpoint, 0, 0);
		endpoint->force_halt = 0;
	}

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_ep_set_wedge(struct usb_ep *ep)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!ep))
		return -EINVAL;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check parameter */
	if (unlikely((endpoint_channel != ENDPOINT_CHANNEL_ENDPOINT0) && (!endpoint->endpoint_descriptor))) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():endpoint %u descriptor is none.\n",
			__FUNCTION__, endpoint_channel);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}
	/* check current queue execute */
	if (!list_empty(&endpoint->queue)) {
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u queue is execute.\n", endpoint_channel);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EAGAIN;
	}

	/* set force halt */
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u transfer is wedged.\n", endpoint_channel);
	f_usb20hdc_halt_transfer(endpoint, 1U, 1U);

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_ep_fifo_status(struct usb_ep *ep)
{
	fusb_func_trace(" ");

	return 0;
}

static void f_usb20hdc_udc_ep_fifo_flush(struct usb_ep *ep)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	struct f_usb20hdc_udc_endpoint *endpoint;
	unsigned char endpoint_channel;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!ep))
		return;

	/* get parameter */
	endpoint = container_of(ep, struct f_usb20hdc_udc_endpoint, endpoint);
	endpoint_channel = endpoint->endpoint_channel;
	f_usb20hdc = endpoint->f_usb20hdc;

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* initialize endpoint FIFO */
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "endpoint %u FIFO is flush.\n", endpoint_channel);
	f_usb20hdc_initialize_endpoint(endpoint, 1U, 0, 0);

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return;
}

static int f_usb20hdc_udc_gadget_get_frame(struct usb_gadget *gadget)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	unsigned short frame_number;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!gadget))
		return -EINVAL;

	/* get a device driver parameter */
	f_usb20hdc = container_of(gadget, struct f_usb20hdc_udc, gadget);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* get frame number */
	frame_number = get_timstamp(f_usb20hdc->register_base_address);
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "frame number is %u.\n", frame_number);

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return (int)frame_number;
}

static int f_usb20hdc_udc_gadget_wakeup(struct usb_gadget *gadget)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!gadget))
		return -EINVAL;

	/* get a device driver parameter */
	f_usb20hdc = container_of(gadget, struct f_usb20hdc_udc, gadget);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check device state */
	if (f_usb20hdc->device_state != USB_STATE_SUSPENDED) {
		hdc_err(f_usb20hdc->gadget.dev.parent, "%s():device state is invalid at %u.\n",
			__FUNCTION__, f_usb20hdc->device_state);
		f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
		hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
		return -EPROTO;
	}

	/* output resume */
	set_reqresume(f_usb20hdc->register_base_address, 1U);
	hdc_dbg(f_usb20hdc->gadget.dev.parent, "resume is output.\n");

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_gadget_set_selfpowered(struct usb_gadget *gadget, int is_selfpowered)
{
	fusb_func_trace(" ");

	return 0;
}

static int f_usb20hdc_udc_gadget_vbus_session(struct usb_gadget *gadget, int is_active)
{
	fusb_func_trace(" ");

	return 0;
}

static int f_usb20hdc_udc_gadget_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
	fusb_func_trace(" ");

	return 0;
}

static int f_usb20hdc_udc_gadget_pullup(struct usb_gadget *gadget, int is_on)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	unsigned long flags;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!gadget))
		return -EINVAL;

	/* get a device driver parameter */
	f_usb20hdc = container_of(gadget, struct f_usb20hdc_udc, gadget);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is started.\n", __FUNCTION__);

	/* get spin lock and disable interrupt, save interrupt status */
	f_usb_spin_lock_irqsave(f_usb20hdc, flags);

	/* check connect request */
	if (is_on) {
		/* check bus connect status */
		if (!f_usb20hdc->bus_connect) {
			hdc_err(f_usb20hdc->gadget.dev.parent, "%s():D+ terminal can not pull-up.\n", __FUNCTION__);
			f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
			return -EINVAL;
		}
		f_usb20hdc_pullup_flag = 1U;

		/* set bus disconnect detect */
		set_bus_connect_detect(f_usb20hdc->register_base_address, 0);

		/* enable host connect */
		f_usb20hdc_enable_host_connect(f_usb20hdc, 1U);
	} else {
		/* check bus connect status */
		if (!f_usb20hdc->bus_connect) {
			f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);
			hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);
			return 0;
		}

		/* disable host connect */
		f_usb20hdc_enable_host_connect(f_usb20hdc, 0);

		/* set bus connect detect */
		set_bus_connect_detect(f_usb20hdc->register_base_address, 1);

		f_usb20hdc_pullup_flag = 0;

		/* notify disconnect to gadget driver */
		f_usb_spin_unlock(f_usb20hdc);
		if ((f_usb20hdc->gadget_driver) && (f_usb20hdc->gadget_driver->disconnect))
			f_usb20hdc->gadget_driver->disconnect(&f_usb20hdc->gadget);

		f_usb_spin_lock(f_usb20hdc);
	}

	/* release spin lock and enable interrupt, return interrupt status */
	f_usb_spin_unlock_irqrestore(f_usb20hdc, flags);

	hdc_dbg(f_usb20hdc->gadget.dev.parent, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_gadget_ioctl(struct usb_gadget *gadget, unsigned code, unsigned long param)
{
	struct f_usb20hdc_udc *f_usb20hdc;
	int ret = 0;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!gadget))
		return -EINVAL;

	/* get a device driver parameter */
	f_usb20hdc = container_of(gadget, struct f_usb20hdc_udc, gadget);

	switch(code) {
	case USB_IOCTL_FIFO_RESET:
	case USB_IOCTL_FIFO_MAP:
	case USB_IOCTL_FIFO_DUMP:
		break;
	case USB_IOCTL_TESTMODE:
		switch(param) {
		case USB_IOCTL_TESTMODE_NORMAL:
			set_testp(f_usb20hdc->register_base_address, 0);
			set_testj(f_usb20hdc->register_base_address, 0);
			set_testk(f_usb20hdc->register_base_address, 0);
			set_testse0nack(f_usb20hdc->register_base_address, 0);
			break;
		case USB_IOCTL_TESTMODE_TEST_J:
			/* set Test_J */
			set_testj(f_usb20hdc->register_base_address, 1U);
			break;
		case USB_IOCTL_TESTMODE_TEST_K:
			/* set Test_K */
			set_testk(f_usb20hdc->register_base_address, 1U);
			break;
		case USB_IOCTL_TESTMODE_SE0_NAK:
			/* set Test_SE_NAK */
			set_testse0nack(f_usb20hdc->register_base_address, 1U);
			break;
		case USB_IOCTL_TESTMODE_TEST_PACKET:
			set_testp(f_usb20hdc->register_base_address, 1U);
			break;
		default:
			ret = -ENOIOCTLCMD;
			break;
		}
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	fusb_func_trace("END");

	return ret;
}

/* So far, it is implementation of F_USB20HDC UDC device driver upper layer interface        */
/************************************************************************************************/

/************************************************************************************************/
/* Implementation of here to F_USB20HDC UDC device driver probing / removing / others        */

static int f_usb20hdc_udc_otg_probe(void *data)
{
	struct f_usb20hdc_udc *f_usb20hdc = (struct f_usb20hdc_udc *)data;
	unsigned long counter;
	int result;

	fusb_func_trace("START");

	f_usb20hdc->bus_connect = is_bus_connected(f_usb20hdc->register_base_address);

	/* enable F_USB20HDC controller */
	enable_controller(f_usb20hdc->register_base_address, 1U);
	enable_dma_controller(f_usb20hdc->register_base_address, 1U);
	enable_bus_connect_detect_controller(f_usb20hdc->register_base_address, 1U);
	enable_dplus_line_pullup_controller(f_usb20hdc->register_base_address, 1U);

	/* pre-initialize F_USB20HDC controller */
	f_usb20hdc_initialize_controller(f_usb20hdc, 1U);

	if (f_usb20hdc->highspeed_support) {
		if (fusb_otg_gadget_get_hs_enable() == USB_OTG_HS_DISABLE) {
			set_reqspeed(f_usb20hdc->register_base_address, REQ_SPEED_FULL_SPEED);
		}
	}

	/* entry a F_USB20HDC device IRQ */
	f_usb20hdc->irq = F_USBHDC_C_INTR_IRQ;
	result = request_irq(f_usb20hdc->irq, f_usb20hdc_on_usb_function, get_irq_flag(), "f_usb20hdc_udc", f_usb20hdc);
	if (result) {
		f_usb20hdc->irq = 0;
		kfree(f_usb20hdc);
		return result;
	}

	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_DMA_CHANNELS; counter++) {
		/* entry a F_USB20HDC DMAC device IRQ */
		f_usb20hdc->dma_irq[counter] = F_USB20HDC_DMA_IRQ[counter];
		result = request_irq(f_usb20hdc->dma_irq[counter], f_usb20hdc_on_end_dma_transfer, get_dma_irq_flag(counter), "f_usb20hdc_dmac_udc", f_usb20hdc);
		if (result) {
			free_irq(f_usb20hdc->irq, f_usb20hdc);
			f_usb20hdc->irq = 0;
			f_usb20hdc->dma_irq[counter] = 0;
			for (; counter; counter--) {
				free_irq(f_usb20hdc->dma_irq[counter - 1], f_usb20hdc);
				f_usb20hdc->dma_irq[counter - 1] = 0;
			}
			kfree(f_usb20hdc);
			return result;
		}
	}

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_otg_remove(void *data)
{
	struct f_usb20hdc_udc *f_usb20hdc = (struct f_usb20hdc_udc *)data;
	unsigned long counter;

	fusb_func_trace("START");

	/* disable F_USB20HDC controller interrupt */
	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_DMA_CHANNELS; counter++) {
		if (f_usb20hdc->dma_irq[counter]) {
			free_irq(f_usb20hdc->dma_irq[counter], f_usb20hdc);
			f_usb20hdc->dma_irq[counter] = 0;
		}
	}

	if (f_usb20hdc->irq) {
		free_irq(f_usb20hdc->irq, f_usb20hdc);
		f_usb20hdc->irq = 0;
	}

	set_soft_reset(f_usb20hdc->register_base_address);

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_otg_resume(void *data)
{
	struct f_usb20hdc_udc *f_usb20hdc = (struct f_usb20hdc_udc *)data;

	/* pre-initialize F_USB20HDC controller */
	f_usb20hdc_initialize_controller(f_usb20hdc, 1U);

	/* enable communicate */
	f_usb20hdc_enable_communicate(f_usb20hdc, 1U);

	return 0;
}

static void f_usb20hdc_udc_release(struct device *dev)
{
	/* current non process */
	return;
}

static int f_usb20hdc_udc_probe(struct platform_device *pdev)
{
	static const struct usb_ep_ops f_usb20hdc_udc_ep_ops = {
		.enable 		= f_usb20hdc_udc_ep_enable,
		.disable		= f_usb20hdc_udc_ep_disable,
		.alloc_request		= f_usb20hdc_udc_ep_alloc_request,
		.free_request		= f_usb20hdc_udc_ep_free_request,
		.queue			= f_usb20hdc_udc_ep_queue,
		.dequeue		= f_usb20hdc_udc_ep_dequeue,
		.set_halt		= f_usb20hdc_udc_ep_set_halt,
		.set_wedge		= f_usb20hdc_udc_ep_set_wedge,
		.fifo_status		= f_usb20hdc_udc_ep_fifo_status,
		.fifo_flush		= f_usb20hdc_udc_ep_fifo_flush,
	};
	static const struct usb_gadget_ops f_usb20hdc_udc_gadget_ops = {
		.get_frame		= f_usb20hdc_udc_gadget_get_frame,
		.wakeup 		= f_usb20hdc_udc_gadget_wakeup,
		.set_selfpowered	= f_usb20hdc_udc_gadget_set_selfpowered,
		.vbus_session		= f_usb20hdc_udc_gadget_vbus_session,
		.vbus_draw		= f_usb20hdc_udc_gadget_vbus_draw,
		.pullup 		= f_usb20hdc_udc_gadget_pullup,
		.ioctl			= f_usb20hdc_udc_gadget_ioctl,
	};
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc;
	struct resource *resource;
	void __iomem *register_base_address;
	int irq;
	void __iomem *dmac_register_base_address[F_USB20HDC_UDC_CONFIG_DMA_CHANNELS];
	int dma_irq[F_USB20HDC_UDC_CONFIG_DMA_CHANNELS];

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!pdev))
		return -EINVAL;

	hdc_dbg(&pdev->dev, "%s() is started.\n", __FUNCTION__);

	/* get a resource for a F_USB20HDC device */
	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!resource) {
		hdc_err(&pdev->dev, "%s():platform_get_resource() is failed.\n", __FUNCTION__);
		hdc_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	register_base_address = (void __iomem *)fusb_otg_get_base_addr(FUSB_ADDR_TYPE_GADGET);

	if (!register_base_address) {
		hdc_err(&pdev->dev, "%s():remap_iomem_region() is failed.\n", __FUNCTION__);
		hdc_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	irq = F_USBHDC_C_INTR_IRQ;

	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_DMA_CHANNELS; counter++) {
		dmac_register_base_address[counter] = (void __iomem *)fusb_otg_get_base_addr(FUSB_ADDR_TYPE_HDMAC);
		if (!dmac_register_base_address[counter]) {
			hdc_err(&pdev->dev, "%s():remap_iomem_region() for F_USB20HDC DMAC %u is failed.\n",
				__FUNCTION__, (unsigned int)counter);
			hdc_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
			return -ENODEV;
		}
	}

	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_DMA_CHANNELS; counter++) {
		/* get an IRQ for a F_USB20HDC DMAC device */
		dma_irq[counter] = F_USB20HDC_DMA_IRQ[counter];
	}

	/* check endpoint buffer size */
	if (!f_usb20hdc_is_endpoint_buffer_usable()) {
		hdc_err(&pdev->dev, "%s():F_USB20HDC endpoint RAM buffer is insufficient.\n", __FUNCTION__);
		hdc_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* allocate F_USB20HDC UDC device driver structure data memory */
	f_usb20hdc = kzalloc(sizeof(*f_usb20hdc), GFP_KERNEL);
	if (!f_usb20hdc) {
		hdc_err(&pdev->dev, "%s():kzalloc() is failed.\n", __FUNCTION__);
		hdc_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -ENOMEM;
	}

	/* initialize the device structure */
	device_initialize(&f_usb20hdc->gadget.dev);

	/* initialize a F_USB20HDC UDC device driver structure */
	f_usb20hdc->gadget.ops = &f_usb20hdc_udc_gadget_ops;
	f_usb20hdc->gadget.ep0 = &f_usb20hdc->endpoint[0].endpoint;
	INIT_LIST_HEAD(&f_usb20hdc->gadget.ep0->ep_list);
	INIT_LIST_HEAD(&f_usb20hdc->gadget.ep_list);
	f_usb20hdc->gadget.speed = USB_SPEED_UNKNOWN;
	f_usb20hdc->gadget.is_dualspeed = 1U;
	f_usb20hdc->gadget.is_otg = 0;
	f_usb20hdc->gadget.is_a_peripheral = 0;
	f_usb20hdc->gadget.b_hnp_enable = 0;
	f_usb20hdc->gadget.a_hnp_support = 0;
	f_usb20hdc->gadget.a_alt_hnp_support = 0;
	f_usb20hdc->gadget.name = "f_usb20hdc_udc";
	f_usb20hdc->gadget.dev.parent = &pdev->dev;
	dev_set_name(&f_usb20hdc->gadget.dev, "gadget");
	f_usb20hdc->gadget.dev.dma_mask = pdev->dev.dma_mask;
	f_usb20hdc->gadget.dev.release = f_usb20hdc_udc_release;
	f_usb20hdc->gadget_driver = NULL;
	spin_lock_init(&f_usb20hdc->lock);
	f_usb20hdc->lock_cnt = 0;
	f_usb20hdc->resource = resource;
	f_usb20hdc->register_base_address = register_base_address;
	f_usb20hdc->irq = irq;
	f_usb20hdc_initialize_endpoint_configure(f_usb20hdc);
	for (counter = ENDPOINT_CHANNEL_ENDPOINT0; counter < F_USB20HDC_UDC_CONFIG_ENDPOINT_CHANNELS; counter++) {
		f_usb20hdc->endpoint[counter].endpoint.driver_data = NULL;
		f_usb20hdc->endpoint[counter].endpoint.ops = &f_usb20hdc_udc_ep_ops;
		list_add_tail(&f_usb20hdc->endpoint[counter].endpoint.ep_list,
			      counter == ENDPOINT_CHANNEL_ENDPOINT0 ? &f_usb20hdc->gadget.ep0->ep_list : &f_usb20hdc->gadget.ep_list);
		f_usb20hdc->endpoint[counter].f_usb20hdc = f_usb20hdc;
		f_usb20hdc->endpoint[counter].endpoint_descriptor = NULL;
		f_usb20hdc->endpoint[counter].request = NULL;
		INIT_LIST_HEAD(&f_usb20hdc->endpoint[counter].queue);
		f_usb20hdc->endpoint[counter].halt = 0;
		f_usb20hdc->endpoint[counter].force_halt = 0;
		f_usb20hdc->endpoint[counter].null_packet_transfer = 0;
		f_usb20hdc->endpoint[counter].dma_transfer = 0;
		if (f_usb20hdc->endpoint[counter].dma_channel != -1)
			f_usb20hdc->endpoint[counter].dma_transfer_buffer_address =
				(void __iomem *)get_epbuf_dma_address((void __iomem *)resource->start, f_usb20hdc->endpoint[counter].dma_channel,
						f_usb20hdc->endpoint[counter].buffer_address_offset[0]);
		else
			f_usb20hdc->endpoint[counter].dma_transfer_buffer_address = NULL;
	}
	f_usb20hdc->device_driver_register = 0;
	f_usb20hdc->highspeed_support = 1U;
	f_usb20hdc->bus_connect = 0;
	f_usb20hdc->selfpowered = 1U;
	f_usb20hdc->device_state = USB_STATE_NOTATTACHED;
	f_usb20hdc->device_state_last = USB_STATE_NOTATTACHED;
	f_usb20hdc->configure_value_last = 0;
	f_usb20hdc->control_transfer_stage = CONTROL_TRANSFER_STAGE_SETUP;
	f_usb20hdc->control_transfer_priority_direction = 1U;
	f_usb20hdc->control_transfer_status_stage_delay = 0;
	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_USB_DMA_CHANNELS; counter++)
		f_usb20hdc->usb_dma_endpoint_channel[counter] = 0;
	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_DMA_CHANNELS; counter++) {
		f_usb20hdc->dmac_register_base_address[counter] = dmac_register_base_address[counter];
		f_usb20hdc->dma_irq[counter] = dma_irq[counter];
		f_usb20hdc->dma_endpoint_channel[counter] = 0;
	}
	f_usb20hdc_data = f_usb20hdc;

	/* setup the private data of a device driver */
	platform_set_drvdata(pdev, f_usb20hdc);

	/* driver registering log output */
	dev_info(&pdev->dev, "F_USB20HDC UDC driver (version %s) is registered.\n", F_USB20HDC_UDC_CONFIG_DRIVER_VERSION);

	hdc_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);

	hcd_otg_ops.pdev	= pdev;
	hcd_otg_ops.data	= (void *)f_usb20hdc;
	hcd_otg_ops.probe	= f_usb20hdc_udc_otg_probe;
	hcd_otg_ops.remove	= f_usb20hdc_udc_otg_remove;
	hcd_otg_ops.resume	= f_usb20hdc_udc_otg_resume;
	hcd_otg_ops.bus_connect = f_usb20hdc_on_detect_bus_connect;

	fusb_otg_gadget_bind(&hcd_otg_ops);

	fusb_func_trace("END");

	return 0;
}

static int f_usb20hdc_udc_remove(struct platform_device *pdev)
{
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!pdev))
		return -EINVAL;

	hdc_dbg(&pdev->dev, "%s() is started.\n", __FUNCTION__);

	/* get a device driver parameter */
	f_usb20hdc = platform_get_drvdata(pdev);
	if (!f_usb20hdc) {
		hdc_err(&pdev->dev, "%s():platform_get_drvdata() is failed.\n", __FUNCTION__);
		hdc_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);
		return -EINVAL;
	}

	/* disable F_USB20HDC controller interrupt */
	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_DMA_CHANNELS; counter++) {
		if (f_usb20hdc->dma_irq[counter]) {
			free_irq(f_usb20hdc->dma_irq[counter], f_usb20hdc);
			f_usb20hdc->dma_irq[counter] = 0;
		}
	}

	if (f_usb20hdc->irq) {
		free_irq(f_usb20hdc->irq, f_usb20hdc);
		f_usb20hdc->irq = 0;
	}

	/* disable communicate */
	f_usb20hdc_enable_communicate(f_usb20hdc, 0);

	/* disable F_USB20HDC controller */
	enable_controller(f_usb20hdc->register_base_address, 0);
	enable_dma_controller(f_usb20hdc->register_base_address, 0);
	enable_bus_connect_detect_controller(f_usb20hdc->register_base_address, 0);
	enable_dplus_line_pullup_controller(f_usb20hdc->register_base_address, 0);

	/* notify disconnect to gadget driver */
	if ((f_usb20hdc->gadget_driver) && (f_usb20hdc->gadget_driver->disconnect))
		f_usb20hdc->gadget_driver->disconnect(&f_usb20hdc->gadget);

	/* notify unbind to gadget driver */
	if ((f_usb20hdc->gadget_driver) && (f_usb20hdc->gadget_driver->unbind))
		f_usb20hdc->gadget_driver->unbind(&f_usb20hdc->gadget);

	/* delete device */
	if (f_usb20hdc->device_driver_register)
		device_del(&f_usb20hdc->gadget.dev);

	/* release device resource */
	platform_set_drvdata(pdev, NULL);

	/* free F_USB20HDC UDC device driver structure data memory */
	kfree(f_usb20hdc);

	/* driver deregistering log output */
	dev_info(&pdev->dev, "F_USB20HDC UDC driver is deregistered.\n");

	hdc_dbg(&pdev->dev, "%s() is ended.\n", __FUNCTION__);

	fusb_func_trace("END");

	return 0;
}

int f_usb20hdc_udc_suspend(struct platform_device *pdev, pm_message_t state)
{
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!pdev))
		return -EINVAL;

	hdc_dbg(&pdev->dev, "%s() is started.\n", __FUNCTION__);

	/* get a device driver parameter */
	f_usb20hdc = platform_get_drvdata(pdev);

	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_DMA_CHANNELS; counter++) {
		disable_irq(f_usb20hdc->dma_irq[counter]);
	}
	disable_irq(f_usb20hdc->irq);

	/* disable communicate */
	f_usb20hdc_enable_communicate(f_usb20hdc, 0);

	/* notify disconnect to gadget driver */
	if ((f_usb20hdc->gadget_driver) && (f_usb20hdc->gadget_driver->disconnect))
		f_usb20hdc->gadget_driver->disconnect(&f_usb20hdc->gadget);

	fusb_func_trace("END");

	return 0;
}

int f_usb20hdc_udc_resume(struct platform_device *pdev)
{
	unsigned long counter;
	struct f_usb20hdc_udc *f_usb20hdc;

	fusb_func_trace("START");

	/* check argument */
	if (unlikely(!pdev))
		return -EINVAL;

	hdc_dbg(&pdev->dev, "%s() is started.\n", __FUNCTION__);

	fusb_otg_resume_initial_set();

	/* get a device driver parameter */
	f_usb20hdc = platform_get_drvdata(pdev);

	if (f_usb20hdc->highspeed_support) {
		if (fusb_otg_gadget_get_hs_enable() == USB_OTG_HS_DISABLE) {
			set_reqspeed(f_usb20hdc->register_base_address, REQ_SPEED_FULL_SPEED);
		}
	}

	enable_irq(f_usb20hdc->irq);
	for (counter = 0; counter < F_USB20HDC_UDC_CONFIG_DMA_CHANNELS; counter++) {
		enable_irq(f_usb20hdc->dma_irq[counter]);
	}

	fusb_func_trace("END");

	return 0;
}

/* So far, it is implementation of F_USB20HDC UDC device driver probing / removing / others    */
/************************************************************************************************/

/**
 * @brief f_usb20hdc_udc_platform_driver
 *
 * F_USB20HDC UDC platform device driver structure data
 */
static struct platform_driver f_usb20hdc_udc_platform_driver = {
	.remove         = f_usb20hdc_udc_remove,
	.suspend        = f_usb20hdc_udc_suspend,
	.resume         = f_usb20hdc_udc_resume,
	.driver         = {
		.name       = "f_usb20hdc_udc",
		.owner      = THIS_MODULE,
	},
};

int __init f_usb20hdc_udc_init(void)
{
	/*
	 * Since F_USB20HDC USB function controller integrated into system-on-chip processors,
	 * platform_driver_probe() is used.
	 */
	return platform_driver_probe(&f_usb20hdc_udc_platform_driver, f_usb20hdc_udc_probe);
}

static void __exit f_usb20hdc_udc_exit(void)
{
	platform_driver_unregister(&f_usb20hdc_udc_platform_driver);
}
module_exit(f_usb20hdc_udc_exit);

/* F_USB20HDC UDC device module definition */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FUJITSU SEMICONDUCTOR LIMITED.");
MODULE_DESCRIPTION("F_USB20HDC USB function controller driver");
MODULE_ALIAS("platform:f_usb20hdc_udc");
