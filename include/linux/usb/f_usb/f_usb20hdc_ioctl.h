/*
 * include/linux/usb/f_usb/f_usb20hdc_ioctl.h
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

/*--------------------------------------------------------------------------*
 * include/linux/usb/scd/scd_ioctl.h
 *
 * Gadget ioctl defintions
 *
 * Copyright 2010 Sony Corporation
 *
 * This file is part of the HS-OTG Controller Driver SCD.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *---------------------------------------------------------------------------*/

#ifndef _F_USB20HDC_IOCTL_H_
#define _F_USB20HDC_IOCTL_H_

/* Gadget IOCTL code */
#define USB_IOCTL_FIFO_RESET			1U				/**< reset FIFO map		  */
#define USB_IOCTL_FIFO_MAP			2U				/**< map FIFO			  */
#define USB_IOCTL_FIFO_DUMP			3U				/**< dump FIFO settings 	  */
#define USB_IOCTL_TESTMODE			4U				/**< Test mode settings 	  */
#define USB_IOCTL_DMA_SET			5U				/**< DMA transfer enable settings */

#define USB_IOCTL_TESTMODE_NORMAL		0				/**< TESTMODE_NORMAL */
#define USB_IOCTL_TESTMODE_TEST_J		1U				/**< TEST_J	     */
#define USB_IOCTL_TESTMODE_TEST_K		2U				/**< TEST_K	     */
#define USB_IOCTL_TESTMODE_SE0_NAK		3U				/**< SE0_NAK	     */
#define USB_IOCTL_TESTMODE_TEST_PACKET		4U				/**< TEST_PACKET     */
#define USB_IOCTL_TESTMODE_FORCE_ENABLE		5U				/**< FORCE_ENABLE    */

struct usb_ioctl_fifo_param {
	uint8_t bEpAdr;			/**< endpoint address */
	uint16_t wSize;			/**< FIFO size	      */
	uint8_t bType;			/**< specify 2 when double buffer or 1 when single buffer */
};

static inline int
usb_gadget_ioctl(struct usb_gadget *gadget, unsigned code, unsigned long param)
{
	if (!gadget->ops->ioctl)
		return -EOPNOTSUPP;
	return gadget->ops->ioctl(gadget, code, param);
}

struct usb_ioctl_dmaset_param {
	uint8_t bEp_num;			/**< a number of EP [0-0x0F] */
	uint8_t bDma_ena;			/**< DMA transfer is valid or not. 0: disable, 1: enable */
};

#endif  /* _F_USB20HDC_IOCTL_H_ */
