#
# Copyright 2007 Sony Corp.
#

# This Makefile is used to build the user level programs
# which is being used for testing the kprobes, jprobes
# and kretprobes as described in the Documentation/kprobe-tests.txt

# Executable names
SRCS = u-005 u-006 u-007 u-008 u-009 \
	kretp-user-test-valid-001-002 kretp-user-test-invalid-001-002 \
	kretp-user-test-003 kretp-user-test-004 \
	kretp-kp-user-test-003 kretp-kp-user-test-004 \
	syscall-statistics-user-space

CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall

LDFLAGS = -lpthread

all : $(SRCS)

clean :
	rm -f *.o $(SRCS)
