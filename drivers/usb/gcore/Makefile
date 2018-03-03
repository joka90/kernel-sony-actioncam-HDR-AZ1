
KDIR ?= $(PWD)/../../..
PWD := $(shell pwd)

obj-y                           += usb_init.o

obj-$(CONFIG_USB_EVENT)         += usb_event.o

obj-$(CONFIG_USB_OTG_CORE)      += usb_otgcore.o

obj-$(CONFIG_USB_GADGET_CORE)   += usb_gadgetcore.o
usb_gadgetcore-objs             := usb_gcore_main.o \
                                   usb_gcore_setup.o \
                                   usb_gcore_ep.o \
                                   usb_gcore_desc.o \
                                   usb_gcore_bus.o \
                                   usb_gcore_wrapper.o


modules modules_install clean:
	make -C $(KDIR) KBUILD_EXTMOD=$(PWD)
