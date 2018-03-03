/*
 * usb_sony_ext.h
 * 
 * Copyright 2008 Sony Corporation
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

/* <H1> sonymisc character device added to usb host core driver. </H1>
   Define event call back register driver.
*/

#ifndef __USB_SONY_EXT_H__
#define __USB_SONY_EXT_H__

/* Include event type definitions. */
#include <linux/usb/gcore/usb_event.h>

/* <H2> Share with userland. PRE part. </H2> */

/* define event data structures. */

/* USB host core event ID.
*/
enum {
	USB_KEVENT_ID_HOSTCORE_CONNECT=0,
	USB_KEVENT_ID_HOSTCORE_DISCONNECT,
	USB_KEVENT_ID_HOSTCORE_ERROR_GET_STATUS,
	USB_KEVENT_ID_HOSTCORE_ERROR_DEBOUNCE,
	USB_KEVENT_ID_HOSTCORE_ERROR_HUB_PORT_INIT,
	USB_KEVENT_ID_HOSTCORE_ERROR_GET_CONFIGURATION,
	USB_KEVENT_ID_HOSTCORE_ERROR_CHOOSE_CONFIGURATION,
	USB_KEVENT_ID_HOSTCORE_ERROR_SET_CONFIGURATION,
};

/*! USB Host Speed Number.
    @note USB_HOSTCORE_SPEED_UNKNOWN means unknown link speed.
    SPEED_UNKNOWN appears when connecting over air so called 
    WUSB (wireless-USB). But NOT TESTED and NOT appers.
    When we supports WUSB host/root hub/inter hub, test 
    SPEED_UNKNOWN.
*/
enum {
	USB_HOSTCORE_SPEED_LS=0,	/*!< Low Speed.  1.5Mbps		*/
	USB_HOSTCORE_SPEED_FS,		/*!< Full Speed.  12Mbps		*/
	USB_HOSTCORE_SPEED_HS,		/*!< High Speed. 480Mbps		*/
	USB_HOSTCORE_SPEED_UNKNOWN,	/*!< Unknown Speed. WUSB wireless USB.	*/
};

/* USB host core callbacks. */
struct usb_hostcore_event {
	void (*connect)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	void (*disconnect)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	/* Errors */
	void (*error_get_status)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	void (*error_debounce)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	void (*error_hub_port_init)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	void (*error_get_configuration)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	void (*error_choose_configuration)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	void (*error_set_configuration)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
};

/*! ioctl USB_IOC_HOSTCORE_PROBE command argument.
	Register USB event handler with hndl.
	Register Callback functions.
*/
struct usb_hostcore_probe_info {
	usb_hndl_t	hndl;					/*!< USB Event handle.				*/
	struct usb_hostcore_event	event;	/*!< Event callback function table.	*/
};


/* Device Descriptor with spped, address. 
   Connect success event information.

   NOTE: Assuming one port(root hub) on system.
*/
struct usb_connect_device {
	unsigned char   speed;			/*!< Connection speed.			*/
	unsigned char   address;		/*!< USB device Address.		*/
	unsigned char   num_interface;	/*!< The number of Interfaces	*/
	unsigned short  vendor_id;		/*!< VendorId					*/
	unsigned short  product_id;		/*!< ProductId					*/
	unsigned short  bcd_device;		/*!< bcdDevice					*/
	unsigned char   max_power;		/*!< MaxPower					*/
	unsigned char   dev_class;		/*!< Device Class				*/
	unsigned char   dev_sub_class;	/*!< Device SubClass			*/
	unsigned char   dev_protocol;	/*!< Device Prorocol			*/
	unsigned char   manufacturer_string_size;
	unsigned char   product_string_size;
	unsigned char   serial_string_size;
};

/* Device error event information.
   Connect error event information.

   NOTE: Assuming one port(root hub) on system.
*/
struct usb_error_device {
	unsigned char	address;		/*!< usb device address */
};

/* Hub error event information.
   Connect error event information.

   NOTE: Assuming one port(root hub) on system.
*/
struct usb_error_hub {
	unsigned char	address;		/*!< usb hub address.		*/
	unsigned char	port;			/*!< usb hub port number.	*/
};

/* Intaface Descriptor.
   Connect event information.
*/
struct usb_connect_interface {
	unsigned char  interface;		/*!< Interface Number		*/
	unsigned char  alternate;		/*!< Alternate Number		*/
	unsigned char  intf_class;		/*!< Interface Class		*/
	unsigned char  intf_sub_class;	/*!< Interface Sub Class	*/
	unsigned char  intf_protocol;	/*!< Interface Protocol		*/
};

/* USB device address.
   Disconnect event information.
   NOTE: Assuming one port(root hub) on system.
*/
struct usb_disconnect_device_info {
	unsigned char  address;	/*!< USB device address. */
};

/*! sonymisc character device node number.
    @note: Major is 240.
*/
#define	USB_HOSTCORE_MINOR_NUMBER	(162)

/*! sonymisc character device ioctl command number base.
*/
#define	USB_IOC_HOSTCORE_BASE		(0xec)
/* Register event callbacks. */
#define	USB_IOC_HOSTCORE_PROBE	\
			_IOW(USB_IOC_HOSTCORE_BASE, 1, struct usb_hostcore_probe_info)
/* UnRegister event callbacks. */
#define	USB_IOC_HOSTCORE_REMOVE	\
			_IO (USB_IOC_HOSTCORE_BASE, 2)

/* Kernel privates. */
#ifdef __KERNEL__
#if (defined(CONFIG_USB_SONY_EXT))

#ifdef CONFIG_OSAL_UDIF
#include <linux/udif/cdev.h>
#include <mach/udif/devno.h>
#else
#include <asm/arch/sonymisc.h>
#endif

#include <linux/usb.h>

#define	USB_SONY_EXT_MINOR_NUMBER		(USB_HOSTCORE_MINOR_NUMBER)

/* Debug macros. */
#if (defined(CONFIG_USB_SONY_EXT_DEBUG))
#define PRINTK_SONY_EXT(format, args...) \
	do {	\
		printk(KERN_INFO "%s: " format, __FUNCTION__,  ## args); \
	} while (0)

#define PRINTK_SONY_EXT_SIMPLE(format, args...) \
	do {	\
		printk(format,  ## args); \
	} while (0)

#else /* (defined(CONFIG_USB_SONY_EXT_DEBUG)) */
#define PRINTK_SONY_EXT(format, args...) \
	if (0 /* false */) {	\
		printk(KERN_INFO "%s: " format, __FUNCTION__,  ## args); \
	}

#define PRINTK_SONY_EXT_SIMPLE(format, args...) \
	if (0 /* false */) {	\
		printk(format,  ## args); \
	}
#endif /* (defined(CONFIG_USB_SONY_EXT_DEBUG)) */

#define USB_SONY_EXT_NAME			"usbcore"
#define MYDRIVER_NAME				USB_SONY_EXT_NAME

#define USB_SONY_EXT_EVENT_HANDLE_INVALID	(0)

/*! Atomic test and set flag. Implements open only one at same time. */
#define USB_SONY_EXT_EXCLUSIVE_OPEN		(0x01)
/*! sonymisc device registered. */
#define USB_SONY_EXT_SONYMISC_REGISTERD	(0x02)

/*! Character device fops for connecting usb host core to usb event.
*/
struct usb_sony_ext {
	unsigned long int				flags;		/*!< only one process can open. */
	struct semaphore				sem_event;	/*!< flags ioctl and event issue. */
#ifdef CONFIG_OSAL_UDIF
	struct cdev				cdev_udif;
#else
	struct sonymisc_device			cdev_sonymisc;
#endif
	struct usb_hostcore_probe_info	event_callbacks;
};

/* function prototypes. */
int usb_sony_ext_event_connect(struct usb_device *udev,struct usb_host_config *hconfig);
int usb_sony_ext_event_disconnect(struct usb_device *udev);


#endif /* (defined(CONFIG_USB_SONY_EXT)) */
#endif /* __KERNEL__ */

/* Shares userland applications. POST part. */

#endif /* __USB_SONY_EXT_H__ */
