/*
 * scsi_sony_ext.h
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

/* <H1> sonymisc character device added to scsi mid layer driver. </H1>
   Define event call back register driver.
*/

#ifndef __SCSI_SONY_EXT_H__
#define __SCSI_SONY_EXT_H__

/* Include event type definitions. */
#include <linux/usb/gcore/usb_event.h>

/* <H2> Share with userland. PRE part. </H2> */

/* define macros */
#define	SCSI_SONY_EXT_HOST_UNKNOWN	(~0UL)

/* define event data structures. */

/* SCSI host event ID.
*/
enum {
	SCSI_SONY_EXT_KEVENT_ID_CONNECT_NODE=0,
	SCSI_SONY_EXT_KEVENT_ID_DISCONNECT_NODE,
	SCSI_SONY_EXT_KEVENT_ID_ERROR_INQUIRY,
	SCSI_SONY_EXT_KEVENT_ID_MAX_LUN,
};


/* SCSI callbacks. */
struct scsi_sony_ext_event {
	void (*connect_node)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	void (*disconnect_node)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	/* Errors */
	void (*error_inquiry)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
	/* Max LUN */
	void (*max_lun)(usb_hndl_t hndl,usb_kevent_id_t id, unsigned char len, void *data);
};

/*! ioctl SCSI_SONY_EXT_IOC_PROBE command argument.
	Register USB event handler with hndl.
	Register Callback functions.
*/
struct scsi_sony_ext_probe_info {
	usb_hndl_t					hndl;	/*!< USB Event handle.				*/
	struct scsi_sony_ext_event	event;	/*!< Event callback function table.	*/
};

/*! Connect node event information.
*/
struct scsi_sony_ext_connect_node  {
	unsigned		major;			/*!< sr major node number.	*/
	unsigned		minor;			/*!< sr minor node number.	*/
};

/*! Dsconnect node event information.
*/
struct scsi_sony_ext_disconnect_node  {
	unsigned		major;			/*!< sr major node number.	*/
	unsigned		minor;			/*!< sr minor node number.	*/
};

/*! Inquiry failed event information.
	@note SCSI-Bulkonly USB device seems as follows.
		host=0x00-0xff	(Connected sequence number).
		channel=0x00	(Fixed to zero).
		id=0x00			(Fixed to zero).
		lun=0x00-0xff	(Logincal unit number. For example, This number means
						multi card reader's slot numner.
	@note If We lost Host Adapter information.
			 All members are fixed to 0xff(=SCSI_SONY_EXT_HOST_UNKNOWN).
*/

struct scsi_sony_ext_error_inquiry_hcil {
	unsigned char		host;		/*!< Host number.		*/
	unsigned char		channel;	/*!< Channel number.	*/
	unsigned char		id;			/*!< Id number.			*/
	unsigned char		lun;		/*!< LUN number.		*/
};

/*! sonymisc character device node minor number.
    @note: major number is 240.
*/
#define SCSI_SONYMISC_MINOR_NUMBER			163

/*! sonymisc character device ioctl command base number.
*/
#define	SCSI_SONY_EXT_IOC_BASE		0xed
/* Register event callbacks. */
#define	SCSI_SONY_EXT_IOC_PROBE		\
			_IOW(SCSI_SONY_EXT_IOC_BASE, 1, struct scsi_sony_ext_probe_info)
/* UnRegister event callbacks. */
#define SCSI_SONY_EXT_IOC_REMOVE	\
			_IO (SCSI_SONY_EXT_IOC_BASE, 2)


/* Kernel privates. */
#ifdef __KERNEL__
#if (defined(CONFIG_SCSI_SONY_EXT))

#ifdef CONFIG_OSAL_UDIF
#include <linux/udif/cdev.h>
#include <mach/udif/devno.h>
#else
#include <asm/arch/sonymisc.h>
#endif

/* Debug macros. */
#if (defined(CONFIG_SCSI_SONY_EXT_DEBUG))
#define PRINTK_SONY_EXT(format, args...) \
	do {	\
		printk(KERN_INFO "%s: " format, __FUNCTION__,  ## args); \
	} while (0)

#define PRINTK_SONY_EXT_SIMPLE(format, args...) \
	do {	\
		printk(format,  ## args); \
	} while (0)

#else /* (defined(CONFIG_SCSI_SONY_EXT_DEBUG)) */
#define PRINTK_SONY_EXT(format, args...) \
	if (0 /* false */) {	\
		printk(KERN_INFO "%s: " format, __FUNCTION__,  ## args); \
	}

#define PRINTK_SONY_EXT_SIMPLE(format, args...) \
	if (0 /* false */) {	\
		printk(format,  ## args); \
	}
#endif /* (defined(CONFIG_SCSI_SONY_EXT_DEBUG)) */

#define SCSI_NAME			"scsi"
#define MYDRIVER_NAME		SCSI_NAME

#define SCSI_SONY_EXT_EVENT_HANDLE_INVALID	(0)

/*! Atomic test and set flag. Implements open only one at same time. */
#define SCSI_SONY_EXT_EXCLUSIVE_OPEN		(0x01)
/*! sonymisc device registered. */
#define SCSI_SONY_EXT_SONYMISC_REGISTERD	(0x02)

/*! sonymisc character device works for connecting scsi to usb event.
    @note "usb event" works not only usb, but also scsi, usb, and so on.
*/
struct scsi_sony_ext {
	unsigned long int				flags;		/*!< only one process can open. */
	struct semaphore				sem_event;	/*!< flags ioctl and event issue. */
#ifdef CONFIG_OSAL_UDIF
	struct cdev				cdev_udif;
#else
	struct sonymisc_device			cdev_sonymisc;
#endif
	struct scsi_sony_ext_probe_info	event_callbacks;
};

/* function prototypes. */
int scsi_sony_ext_event_connect_node(unsigned major, unsigned minor);
int scsi_sony_ext_event_disconnect_node(unsigned major, unsigned minor);
int scsi_sony_ext_event_error_inquiry(unsigned host, unsigned channel, unsigned id, unsigned lun);
int scsi_sony_ext_event_max_lun(unsigned int luns);

#endif /* (defined(CONFIG_USB_HOST_SONY_EXT)) */
#endif /* __KERNEL__ */

/* Shares userland applications. POST part. */

#endif /* __SCSI_SONY_EXT_H__ */
