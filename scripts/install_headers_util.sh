#!/bin/sh
#
#  Script to generate install userland headers path
#
#  Copyright 2006-2009 Sony Corporation.
#
set -e # stop on error

usage() {
    echo "$0 [-a|-c|\${objtree}]" > /dev/tty
    echo "    -a: return ARCH " > /dev/tty
    echo "    -c: return CROSS_COMPILE " > /dev/tty
    echo "    -k: return install kbuild header dir " > /dev/tty
    echo "      : return install user header dir " > /dev/tty
    exit 1
}

fatal () {
    echo "FATAL: $*" > /dev/tty
    exit 1
}

while [ $# != 0 ]; do
    case $1 in
      -a)
	RET_ARCH=1
	shift
	;;
      -c)
	RET_CROSS_COMPILE=1
	shift
	;;
      -k)
	RET_INSTALL_KBUILD_HEADER_DIR=1
	shift
	;;
      --help)
	usage
	;;
      *)
	if [ -z "${OBJDIR}" ]; then
	    OBJDIR=$1
	else
	    fatal "too many parameters ${OBJDIR} and $1"
	fi
	shift
	;;
    esac
done

if [ -z "${OBJDIR}" ]; then
    OBJDIR=${objtree}
fi

if [ -n "${RET_ARCH}" ]; then
    if [ -n "${RET_CROSS_COMPILE}" ]; then
	fatal "cannot set -a and -c simultaneously"
    fi
fi

if [ -z "${OBJDIR}" ]; then
    fatal "cannot find \${objtree}"
fi

ARCH=`cat ${OBJDIR}/.arch_name`
CROSS_COMPILE=`cat ${OBJDIR}/.cross_compile`

case ${ARCH} in
	arm)
	    case "${CROSS_COMPILE}" in
		arm-sony-linux-gnueabi-*)
		    INSTDIR=/usr/local/arm-sony-linux-gnueabi;;
		*)
		    INSTDIR=/usr/local/arm-sony-linux;;
	    esac
	    ;;
	i386)
	    INSTDIR=/usr/local/i686-sony-linux
	    ;;
	mips)
	    if [ "${CROSS_COMPILE}" == "mips_nfp-sony-linux-dev-" ]; then
		INSTDIR=/usr/local/mips_nfp-sony-linux
	    else
		INSTDIR=/usr/local/mips-sony-linux
	    fi
	    ;;
	ppc)
	    INSTDIR=/usr/local/powerpc-sony-linux
	    ;;
	powerpc)
	    INSTDIR=/usr/local/powerpc-sony-linux
	    ;;
	*)
	    echo
	    fatal "${ARCH} is not supported"
	    ;;
esac

if [ -n "${RET_ARCH}" ]; then
    echo "${ARCH}"
elif [ -n "${RET_CROSS_COMPILE}" ]; then
    echo "${CROSS_COMPILE}"
elif [ -n "${RET_INSTALL_KBUILD_HEADER_DIR}" ]; then
    echo "${INSTDIR}"
else
    echo "${INSTDIR}/target"
fi


