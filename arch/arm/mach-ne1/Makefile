#
# Makefile for the linux kernel.
#

obj-y                                   := core.o clock.o ne1_gpio.o dma.o
obj-$(CONFIG_HOTPLUG_CPU)               += hotplug.o
obj-$(CONFIG_SMP)                       += headsmp.o platsmp.o
obj-$(CONFIG_LOCAL_TIMERS)              += localtimer.o
obj-$(CONFIG_MACH_NE1TB)                += ne1tb.o
obj-$(CONFIG_MACH_NE1DB)                += ne1db.o

obj-$(CONFIG_NE1_USB)			+= usb.o

