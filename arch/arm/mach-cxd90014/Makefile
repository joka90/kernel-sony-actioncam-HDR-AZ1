#
# Makefile for the linux kernel.
#

obj-y				:= boot.o
obj-y				+= adrs_conv.o
obj-y				+= core.o clock.o cpuid.o
obj-y				+= uart.o
obj-y				+= time.o timer.o
obj-y				+= gic.o
obj-y				+= cfg.o
obj-y				+= wdt.o
obj-y				+= params.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o
obj-$(CONFIG_SMP)		+= platsmp.o
obj-y				+= fiq.o fiq_drv.o
obj-$(CONFIG_OSAL_UDIF)		+= udif/
obj-$(CONFIG_ARCH_CXD90014)	+= cxd90014.o
obj-$(CONFIG_CACHE_PL310)	+= pl310.o
obj-$(CONFIG_LOCAL_TIMERS)	+= localtimer.o
obj-$(CONFIG_OOM_CALLBACK)	+= oom_kill.o
obj-y				+= pcie_setup.o
obj-$(CONFIG_CXD90014_PCIE2_DMX)	+= pcie.o pcie_ctl.o
obj-$(CONFIG_QEMU)	+= qemu.o pci.o
obj-$(CONFIG_THREAD_MONITOR)	+= tmonitor.o
obj-$(CONFIG_EXCEPTION_MONITOR)	+= error.o
obj-$(CONFIG_SUBSYSTEM)		+= subsystem.o
obj-$(CONFIG_PM)		+= pm.o sleep.o resume.o
obj-$(CONFIG_OSAL_UDIF)		+= hwtimer.o
obj-$(CONFIG_EJ_KSLOG)		+= kslog.o
obj-y				+= bam.o
obj-y				+= pmon.o
obj-y				+= vtp.o
obj-$(CONFIG_WARM_BOOT_IMAGE)  += cmpr.o
obj-y				+= rmon.o
obj-$(CONFIG_ARM_DAP)		+= dap.o
obj-$(CONFIG_CXD90014_NANDC)	+= nandc.o


archprepare:

archmrproper:
