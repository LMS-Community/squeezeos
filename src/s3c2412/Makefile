
# Absolute path to top of JIVE directories
SRCDIR = $(realpath ${PWD}/..)
TOPDIR = $(dir ${SRCDIR})

export BUILDTOP=$(TOPDIR)build
export PREFIX=${BUILDTOP}/usr

export CFLAGS=-I${PREFIX}/include -Os
export LDFLAGS=-L${PREFIX}/lib

TARGET=arm-926ejs-linux-gnueabi

export CC=$(TARGET)-gcc
export LD=$(TARGET)-ld
export RANLIB=$(TARGET)-ranlib
export AR=$(TARGET)-ar
export HOST=${TARGET}

export TOOLPATH = $(shell dirname `which gcc`)


# EXTRAVERSION passed from top-level Makefile
KERNEL_SRC=${TOPDIR}/src/kernel${EXTRAVERSION}/linux-2.6.22
MODDIR=${BUILDTOP}/lib/modules/2.6.22${EXTRAVERSION}


.PHONY: all clean kernel kernel-modules mini_fo

all: kernel kernel-modules
clean: kernel-clean modules-clean


#
# Kernel
#
kernel:
	cd ${KERNEL_SRC}; make zImage
	install ${KERNEL_SRC}/arch/arm/boot/zImage ${TOPDIR}/image/zImage${EXTRAVERSION}

kernel-clean:
	cd ${KERNEL_SRC}; make clean


#
# Kernel modules
#
kernel-modules:
	mkdir -p ${MODDIR}
	mkdir -p ${BUILDTOP}/lib/firmware
	mkdir -p ${BUILDTOP}/usr/sbin
	mkdir -p ${BUILDTOP}/usr/local/sbin
	install modules/kernel/gspi8xxx.ko ${MODDIR}/gspi8xxx.ko
	install modules/kernel/gspi.ko ${MODDIR}/gspi.ko
	install modules/lib/firmware/gspi8686.bin ${BUILDTOP}/lib/firmware/gspi8686.bin
	install modules/lib/firmware/helper_gspi.bin ${BUILDTOP}/lib/firmware/helper_gspi.bin
	install modules/usr/local/sbin/wpa_cli ${BUILDTOP}/usr/sbin/wpa_cli
	install modules/usr/local/sbin/wpa_supplicant ${BUILDTOP}/usr/sbin/wpa_supplicant
	install modules/usr/sbin/wlanconfig ${BUILDTOP}/usr/sbin/wlanconfig


modules-clean:
