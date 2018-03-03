
ccflags-$(CONFIG_PM_DEBUG)	:= -DDEBUG

obj-$(CONFIG_PM)		+= main.o pm.o
obj-$(CONFIG_PM_SLEEP)		+= console.o
obj-$(CONFIG_FREEZER)		+= process.o
obj-$(CONFIG_SUSPEND)		+= suspend.o
obj-$(CONFIG_PM_TEST_SUSPEND)	+= suspend_test.o
obj-$(CONFIG_HIBERNATION)	+= hibernate.o snapshot.o swap.o user.o \
				   block_io.o
obj-$(CONFIG_SNSC_SSBOOT)	+= ssboot/

obj-$(CONFIG_MAGIC_SYSRQ)	+= poweroff.o

obj-$(CONFIG_SNSC_PMMEM)	+= snsc_pmmem.o
