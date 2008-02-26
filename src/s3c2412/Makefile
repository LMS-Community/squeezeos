
# Absolute path to top of JIVE directories
SRCDIR = $(realpath ${PWD}/..)
TOPDIR = $(dir ${SRCDIR})

IMAGEDIR=${TOPDIR}image

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


EXTRAVERSION=-P7
KERNEL_SRC=${TOPDIR}/src/s3c2412/linux-2.6.22
BOOTLOADER_SRC=${TOPDIR}/src/s3c2412/s3c24x0_uboot
MODDIR=${BUILDTOP}/lib/modules/2.6.22${EXTRAVERSION}


.PHONY: all clean bootloader fw_env kernel kernel-modules mini_fo

all: bootloader fw_env kernel kernel-modules


#
# Bootloader
#
bootloader: ${IMAGEDIR}/u-boot.bin

${IMAGEDIR}/u-boot.bin: ${IMAGEDIR}
	cd ${BOOTLOADER_SRC} && ${MAKE} smdk2413_config && ${MAKE} && cp u-boot.bin ${IMAGEDIR}


#
# Bootloader utilities
#
fw_env:
	cd ${BOOTLOADER_SRC}/tools/env && ${MAKE} CFLAGS="${CFLAGS} -I ${KERNEL_SRC}/include"
	install ${BOOTLOADER_SRC}/tools/env/fw_printenv ${BUILDDIR}/usr/sbin/fw_printenv
	cd ${BUILDDIR}/usr/sbin && ln -fs fw_printenv fw_setenv


#
# Kernel
#
kernel:
	cd ${KERNEL_SRC}; make zImage EXTRAVERSION=${EXTRAVERSION}
	install ${KERNEL_SRC}/arch/arm/boot/zImage ${TOPDIR}/image/zImage${EXTRAVERSION}


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


# Clean
clean:
	cd ${KERNEL_SRC}; make clean
	cd ${BOOTLOADER_SRC} ; make clean
