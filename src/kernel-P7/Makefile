
# Absolute path to top of JIVE directories
SRCDIR = $(realpath ${PWD}/..)
TOPDIR = $(dir ${SRCDIR})

export DESTDIR=$(TOPDIR)build
export PREFIX=${DESTDIR}/usr

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
MODDIR=${DESTDIR}/lib/modules/2.6.10${EXTRAVERSION}


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
	mkdir -p ${DESTDIR}/lib/firmware
	mkdir -p ${DESTDIR}/usr/sbin
	mkdir -p ${DESTDIR}/usr/local/sbin
	install modules/kernel/gspi8686.ko ${MODDIR}/gspi8686.ko
	install modules/kernel/gspi.ko ${MODDIR}/gspi.ko
	install modules/kernel/lis302dl.ko ${MODDIR}/lis302dl.ko
	install modules/lib/firmware/gspi8686.bin ${DESTDIR}/lib/firmware/gspi8686.bin
	install modules/lib/firmware/helper_gspi.bin ${DESTDIR}/lib/firmware/helper_gspi.bin
	install modules/usr/local/sbin/wpa_cli ${DESTDIR}/usr/local/sbin/wpa_cli
	install modules/usr/local/sbin/wpa_supplicant ${DESTDIR}/usr/local/sbin/wpa_supplicant
	install modules/usr/sbin/wlanconfig ${DESTDIR}/usr/sbin/wlanconfig


modules-clean:
	-cd mini_fo-0-6-2-pre1; rm *.ko *.o

mini_fo:
	cd mini_fo-0-6-2-pre1; make KERNEL_SRC=${KERNEL_SRC} MODDIR=${MODDIR} install
