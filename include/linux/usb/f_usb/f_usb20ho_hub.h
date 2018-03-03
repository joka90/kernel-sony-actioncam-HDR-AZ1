/*
 * include/linux/usb/f_usb/f_usb20ho_hub.h
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

/*-----------------------------------------------------------------------------
 * include/linux/usb/scd/scd_hub.h
 *
 * Host hub definitions
 *
 * Copyright 2011 Sony Corporation
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

#ifndef _F_USB20HO_HUB_H_
#define _F_USB20HO_HUB_H_

/* Host RootHub Vendor Requests */
#define F_USB20HO_DEVREQ_H2D_V_O	0x43U
#define F_USB20HO_ClearPortFeature	(( F_USB20HO_DEVREQ_H2D_V_O << 8 ) | USB_REQ_CLEAR_FEATURE)
#define F_USB20HO_SetPortFeature	(( F_USB20HO_DEVREQ_H2D_V_O << 8 ) | USB_REQ_SET_FEATURE)

#define F_USB20HO_PORT_FEAT_EP_NO_DMA		1U
#define F_USB20HO_PORT_FEAT_TEST_SE0		2U
#define F_USB20HO_PORT_FEAT_TEST_J		3U
#define F_USB20HO_PORT_FEAT_TEST_K		4U
#define F_USB20HO_PORT_FEAT_TEST_PACKET 	5U
#define F_USB20HO_PORT_FEAT_TEST_SETUP		6U
#define F_USB20HO_PORT_FEAT_TEST_IN		7U
#define F_USB20HO_PORT_FEAT_TEST_FORCE_ENABLE	8U
#define F_USB20HO_PORT_FEAT_TEST_SUSPEND	9U

#endif  /* _F_USB20HO_HUB_H_ */
