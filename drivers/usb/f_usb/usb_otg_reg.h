/*
 * drivers/usb/f_usb/usb_otg_reg.h
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

#ifndef _USB_OTG_REG_H_
#define _USB_OTG_REG_H_

static inline unsigned int get_reg_param(UDIF_VA base_address, unsigned int offset, unsigned int bit)
{
	UDIF_U32 reg_param;

	reg_param = udif_ioread32((UDIF_VA)(base_address + offset));

	return ((unsigned int)reg_param & bit);
}

static inline void set_reg_param(UDIF_VA base_address, unsigned int offset, unsigned int bit)
{
	UDIF_U32 reg_param;

	reg_param = udif_ioread32((UDIF_VA)(base_address + offset));
	reg_param |= bit;
	udif_iowrite32(reg_param, (UDIF_VA)(base_address + offset));

	return;
}

static inline void clear_reg_param(UDIF_VA base_address, unsigned int offset, unsigned int bit)
{
	UDIF_U32 reg_param;

	reg_param = udif_ioread32(base_address + offset);
	reg_param &= ~bit;
	udif_iowrite32(reg_param, (UDIF_VA)(base_address + offset));

	return;
}

static inline void write_reg_param(UDIF_VA base_address, unsigned int offset, unsigned int bit)
{
	udif_iowrite32((UDIF_U32)bit, (UDIF_VA)(base_address + offset));

	return;
}

/* OTG */
#define FUSB_REG_OTG_UNLIMIT			0x0000			/**<  */
#define FUSB_REG_OTG_UNLIMIT_SET		0x0004U			/**<  */
#define FUSB_REG_OTG_UNLIMIT_CLR		0x0008U			/**<  */
#define FUSB_DEF_USB_HF_SEL			(1U << 4)		/**<  */
#define FUSB_REG_OTG_USB_ID_EXT_OFFSET		0x0010U			/**<  */
#define FUSB_DEF_USB_VBUS			(1U << 0)		/**<  */
#define FUSB_DEF_USB_ID 			(1U << 1)		/**<  */
#define FUSB_REG_OTG_USB_INTS_OFFSET		0x0030U			/**<  */
#define FUSB_DEF_USB_VBUS_INTS			(1U << 8)		/**<  */
#define FUSB_DEF_USB_ID_INTS			(1U << 9)		/**<  */
#define FUSB_REG_OTG_USB_INTHE_OFFSET		0x0040U			/**<  */
#define FUSB_DEF_USB_VBUS_INTHE 		(1U << 8)		/**<  */
#define FUSB_DEF_USB_ID_INTHE			(1U << 9)		/**<  */
#define FUSB_REG_OTG_USB_INTLE_OFFSET		0x0050U			/**<  */
#define FUSB_DEF_USB_VBUS_INTLE 		(1U << 8)		/**<  */
#define FUSB_DEF_USB_ID_INTLE			(1U << 9)		/**<  */
#define FUSB_REG_OTG_USB_DEVICE_OFFSET		0x0060U			/**<  */
#define FUSB_DEF_M_HSPEED_O			(1U << 12)		/**<  */

/* EHCI */
#define FUSB_REG_EHCI_BASE_ADDR 		0x0000
#define FUSB_REG_EHCI_PORTSC_1_OFFSET		(FUSB_REG_EHCI_BASE_ADDR + 0x0054U)	/**<  */
#define FUSB_DEF_PORT_POWER			(1U << 12)				/**<  */
#define FUSB_DEF_PORT_OWNER			(1U << 13)				/**<  */

/* OHCI */
#define FUSB_REG_OHCI_BASE_ADDR 		0x1000U
#define FUSB_REG_OHCI_HC_RH_PORT_STATUS 	(FUSB_REG_OHCI_BASE_ADDR + 0x0054U)	/**<  */
#define FUSB_DEF_LOW_SPEED_DEVICE_ATTACHED	(1U << 9)				/**<  */

/* Other Registers */
#define FUSB_REG_OTHER_BASE_ADDR		0x2000U
#define FUSB_REG_OTHER_PHY_MODE_SETTING 	(FUSB_REG_OTHER_BASE_ADDR + 0x0004U)	/**<  */
#define FUSB_DEF_RESET				(1U << 31)				/**<  */

/* Gadget */
#define FUSB_REG_GADGET_CONF_OFFSET		0x0000			/**<  */
#define FUSB_DEF_SOFT_RESET			(1U << 2)		/**<  */
#define FUSB_REG_GADGET_DEVC_OFFSET		0x0200U			/**<  */
#define FUSB_DEF_REQSPEED			(1U << 0)		/**<  */
#define FUSB_DEF_DISCONNECT			(1U << 5)		/**<  */

/* CLKRST_4 */
#define FUSB_REG_IPRESET0_OFFSET		0x00A0U			/**<  */
#define FUSB_REG_IPRESET0_SET_OFFSET		0x00A4U			/**<  */
#define FUSB_REG_IPRESET0_CLR_OFFSET		0x00A8U			/**<  */
#define FUSB_DEF_RRST_X2BB			(1U << 0)		/**<  */
#define FUSB_DEF_RRST_USBHO			(1U << 1)		/**<  */
#define FUSB_DEF_RRST_USBHDC			(1U << 2)		/**<  */
#define FUSB_DEF_RRST_USB			(FUSB_DEF_RRST_X2BB | FUSB_DEF_RRST_USBHO | FUSB_DEF_RRST_USBHDC) /**<  */

#endif /* _F_USB_REGISTER_H_ */

