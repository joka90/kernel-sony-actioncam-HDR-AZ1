#!/bin/sh
function mklink
{
  if [ -e $1 -a ! -L $1 ]; then
    echo $1 exists and is not symbolic link.
    return;
  fi
  if [ ! -e $1.org ]; then
    echo $1.org does not exist.
    return;
  fi
  echo symlink $1 to $1.org
  ln -snf $1.org $1
}

if [ ! -e ./MAINTAINERS ]; then
	echo "Please run on top of source tree."
	exit 1
fi

(cd include/linux && mklink usb.h)
(cd drivers       && mklink scsi)
(cd drivers/usb   && mklink Kconfig)
(cd drivers/usb   && mklink Makefile)
(cd drivers/usb   && mklink core)
(cd drivers/usb   && mklink storage)
(cd drivers/usb   && mklink gadget)
(cd drivers/usb   && mklink host)
