
# Absolute path to top of JIVE directories
SRCDIR = $(realpath ${PWD}/..)
TOPDIR = $(dir ${SRCDIR})

export BUILDTOP=$(TOPDIR)build
export PREFIX=${BUILDTOP}/usr

export CFLAGS=-I${PREFIX}/include -Os
export LDFLAGS=-L${PREFIX}/lib

TARGET=arm-9tdmi-linux-gnu

export CC=$(TARGET)-gcc
export LD=$(TARGET)-ld
export RANLIB=$(TARGET)-ranlib
export AR=$(TARGET)-ar
export HOST=${TARGET}

export TOOLPATH = $(shell dirname `which gcc`)


# EXTRAVERSION passed from top-level Makefile
KERNEL_SRC=${TOPDIR}/src/kernel${EXTRAVERSION}/linux-2.6.10
MODDIR=${BUILDTOP}/lib/modules/2.6.10${EXTRAVERSION}


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
kernel-modules: mini_fo
	mkdir -p ${BUILDTOP}/lib/firmware
	mkdir -p ${BUILDTOP}/usr/sbin
	mkdir -p ${BUILDTOP}/usr/local/sbin
	install modules/kernel/gspi8686.ko ${MODDIR}/gspi8686.ko
	install modules/kernel/gspi.ko ${MODDIR}/gspi.ko
	install modules/kernel/lis302dl.ko ${MODDIR}/lis302dl.ko
	install modules/lib/firmware/gspi8686.bin ${BUILDTOP}/lib/firmware/gspi8686.bin
	install modules/lib/firmware/helper_gspi.bin ${BUILDTOP}/lib/firmware/helper_gspi.bin
	install modules/usr/local/sbin/wpa_cli ${BUILDTOP}/usr/local/sbin/wpa_cli
	install modules/usr/local/sbin/wpa_supplicant ${BUILDTOP}/usr/local/sbin/wpa_supplicant
	install modules/usr/sbin/wlanconfig ${BUILDTOP}/usr/sbin/wlanconfig


modules-clean:
	-cd mini_fo-0-6-2-pre1; rm *.ko *.o

mini_fo:
	cd mini_fo-0-6-2-pre1; make KERNEL_SRC=${KERNEL_SRC} MODDIR=${MODDIR} install
