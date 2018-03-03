#
# Makefile for the LTT objects.
#


obj-$(CONFIG_LTT)			+= ltt-core.o
obj-$(CONFIG_LTT_TRACER)		+= ltt-tracer.o
obj-$(CONFIG_LTT_TRACE_CONTROL)	+= ltt-marker-control.o

obj-$(CONFIG_LTT_RELAY)		+= ltt-relay.o
ltt-relay-objs := ltt-relay-lockless.o ltt-relay-alloc.o ltt-relay-splice.o \
		  ltt-relay-vfs.o ltt-event-header.o ltt-ascii.o \
		  ltt-serialize.o ltt-type-serializer.o

obj-$(CONFIG_LTT_STATEDUMP)		+= ltt-statedump.o
obj-$(CONFIG_LTT_TRACE_CONTROL)	+= ltt-trace-control.o
obj-$(CONFIG_LTT_USERSPACE_EVENT)	+= ltt-userspace-event.o
obj-$(CONFIG_LTT_FILTER)		+= ltt-filter.o
obj-$(CONFIG_LTT_KPROBES)		+= ltt-kprobes.o
obj-$(CONFIG_LTT_TRACEPROBES)		+= probes/
