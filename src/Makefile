
# Absolute path to top of JIVE directories
TOPDIR = $(dir ${PWD})

IMAGEDIR=${TOPDIR}image

export BUILDDIR=$(TOPDIR)build
export STAGE=$(TOPDIR)stage
export PREFIX=${BUILDDIR}/usr

export CFLAGS=-I${PREFIX}/include -Os
export LDFLAGS=-L${PREFIX}/lib

TARGET=arm-9tdmi-linux-gnu

export CC=$(TARGET)-gcc
export LD=$(TARGET)-ld
export RANLIB=$(TARGET)-ranlib
export AR=$(TARGET)-ar
export HOST=${TARGET}


KERNEL_SRC=${TOPDIR}/src/kernel-P7/linux-2.6.10
BOOTLOADER_SRC=${TOPDIR}/src/bootloader/s3c24x0_uboot


.PHONY: all bootloader fw_env kernel kernel-P4 kernel-P7 system debug jive image clean distclean

all: bootloader system kernel fw_env jive image


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


kernel: kernel-P4 kernel-P7

kernel-P4: $(IMAGEDIR)
	cd kernel-P4; make -f Makefile EXTRAVERSION=-P4
	# backwards compatibility
	mv ${IMAGEDIR}/zImage-P4 ${IMAGEDIR}/zImage


kernel-P7: $(IMAGEDIR)
	cd kernel-P7; make -f Makefile EXTRAVERSION=-P7


#
# System
#
system:
	cd system; ${MAKE} -f Makefile

debug:
	cd system; ${MAKE} -f Makefile debug


#
# Jive Apps
#
jive:
	cd ../../jive/src/pkg; ${MAKE} -f Makefile.squeezeboxjive TOPDIR=${TOPDIR}


#
# Firmware image
#
image: ${IMAGEDIR}
	# Bump version number
	scripts/mkversion > ${BUILDDIR}/etc/jive.version
	cp ${BUILDDIR}/etc/jive.version ${IMAGEDIR}/jive.version
	# Copy files for cramfs
	-rm -rf ${STAGE}
	mkdir -p ${STAGE}
	cd ${BUILDDIR}; find * -type d -exec mkdir ${STAGE}/{} \;
	cd ${BUILDDIR}; find * \( -name man -or -name include \) -prune -or \! \( -name *.a -or -name *.la -or -type d \) -exec cp -rp {} ${STAGE}/{} \;
	#sudo mknod ${STAGE}/dev/console c 5 1
	#sudo mknod ${STAGE}/dev/ttySAC0 c 204 64
	# Create cramfs
	mkcramfs ${STAGE} ${IMAGEDIR}/root.cramfs
	# Create upgrade.md5
	cd ${IMAGEDIR} && md5sum zImage zImage-P7 root.cramfs > upgrade.md5
	# Create jive.bin
	cd ${IMAGEDIR} && zip ${IMAGEDIR}/jive.bin jive.version upgrade.md5 zImage zImage-P7 root.cramfs
	@cat ${IMAGEDIR}/jive.version


${IMAGEDIR}:
	-mkdir -p ${IMAGEDIR}

#
# Clean
#
clean:
	cd bootloader/s3c24x0_uboot; ${MAKE} clean
	cd kernel-P4; ${MAKE} clean EXTRAVERSION=-P4
	cd kernel-P7; ${MAKE} clean EXTRAVERSION=-P7
	cd system; ${MAKE} -f Makefile clean
	cd ../../jive/src/pkg; ${MAKE} -f Makefile.squeezeboxjive TOPDIR=${TOPDIR} clean

distclean: clean
	rm -rf ${BUILDDIR}
