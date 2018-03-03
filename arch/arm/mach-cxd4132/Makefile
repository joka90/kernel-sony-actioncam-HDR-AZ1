#
# Makefile for the linux kernel.
#

obj-y				:= boot.o
obj-y				+= core.o clock.o
obj-y 				+= cpuid.o
#obj-y 				+= speed.o fcs.o
obj-y				+= time.o timer.o
obj-y				+= uart.o
obj-y				+= gic.o
obj-y				+= cache-pl310.o cache-pl310-init.o
obj-$(CONFIG_LOCAL_TIMERS)      += localtimer.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o
obj-$(CONFIG_SMP)		+= platsmp.o
ifneq ($(CONFIG_QEMU),y)
obj-y				+= sdc.o sdc-zqcal.o
endif
#obj-y				+= fiq.o fiq_drv.o
#obj-y				+= wdt.o
obj-$(CONFIG_OSAL_UDIF)		+= udif/
obj-$(CONFIG_ARCH_CXD4132)	+= cxd4132.o
obj-$(CONFIG_CACHE_PL310S)	+= pl310.o
obj-y				+= pm.o
obj-$(CONFIG_CXD4132_GPIO)	+= gpio.o
#obj-y				+= bam.o
#obj-$(CONFIG_WARM_BOOT_IMAGE)	+= cmpr.o
#obj-y				+= cachectrl.o

archprepare:

archmrproper:
