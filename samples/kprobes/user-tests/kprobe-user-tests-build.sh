# This script builds all the user level tests that should be used for testing
# kprobes, jprobes and kretprobes
# The <arch> option should be either "arm", "mips", "powerpc" or "i386"

#! /bin/sh

if [ "$#" -eq "0" ];
then
	echo "Usage: ./kprobe-user-tests-build.sh <arch> <dialects (arm/armv7a)>"
	exit 0
fi

if [ "$1" == "mips" ];
then
	make ARCH=$1 CROSS_COMPILE=/usr/local/$1-sony-linux/devel/cross/bin/$1-sony-linux-dev- clean
	make ARCH=$1 CROSS_COMPILE=/usr/local/$1-sony-linux/devel/cross/bin/$1-sony-linux-dev-

else
	if [ "$1" == "arm" ]  && [ "$2" == "armv7a" ];
	then
		make ARCH=arm CROSS_COMPILE=/usr/local/arm-sony-linux-gnueabi/cross/devel/bin/arm-sony-linux-gnueabi-armv7a-dev- clean
		make ARCH=arm CROSS_COMPILE=/usr/local/arm-sony-linux-gnueabi/cross/devel/bin/arm-sony-linux-gnueabi-armv7a-dev-
	else
		make ARCH=arm CROSS_COMPILE=/usr/local/arm-sony-linux-gnueabi/cross/devel/bin/arm-sony-linux-gnueabi-dev- clean
		make ARCH=arm CROSS_COMPILE=/usr/local/arm-sony-linux-gnueabi/cross/devel/bin/arm-sony-linux-gnueabi-dev-
	fi
fi
