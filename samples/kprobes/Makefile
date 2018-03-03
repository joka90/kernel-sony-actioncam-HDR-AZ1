#
# Makefile for the Kprobes test modules kernel.
# This builds the kprobes example kernel modules.
# After build, to use the module perform below step
# (as root):  insmod <module_name.ko>
#
# builds the kprobes example kernel modules;
# then to use one (as root):  insmod <module_name.ko>

obj-$(CONFIG_SAMPLE_KPROBES) += kprobe_example.o jprobe_example.o \
				k-001.o k-002.o k-003.o k-004.o k-005.o \
				k-006.o k-007.o k-008.o k-009.o \
				jp-test-001.o jp-test-002.o jp-test-003.o \
				jp-test-004.o jp-test-005.o jp-kp-test-001.o \
				jp-kp-test-002.o jp-kp-test-003.o \
				jp-kp-test-004.o jp-kp-test-005.o
obj-$(CONFIG_SAMPLE_KRETPROBES) += kretprobe_example.o \
				   kretp-test-001.o kretp-test-002.o \
				   kretp-test-003.o kretp-test-004.o \
				   kretp-kp-test-001.o kretp-kp-test-002.o \
				   kretp-kp-test-003.o kretp-kp-test-004.o \
 				   kretp-kp-jp-test-001.o kretp-kp-jp-test-002.o \
 				   syscall-statistics-kprobe-relay.o

