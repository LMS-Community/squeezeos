#!/bin/sh
#
# Called from udev
#
# Start getty for usb-serial-dongles

/sbin/getty ttyUSB0 115200 VC vt100 &

