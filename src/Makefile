
# Absolute path to top of Squeezeos directories
TOPDIR = $(dir ${PWD})

IMAGEDIR=${TOPDIR}image

export BUILDDIR=$(TOPDIR)build
export STAGE=$(TOPDIR)stage
export PREFIX=${BUILDDIR}/usr

export CFLAGS=-I${PREFIX}/include -Os
export LDFLAGS=-L${PREFIX}/lib

TARGET=arm-926ejs-linux-gnueabi

export CC=$(TARGET)-gcc
export LD=$(TARGET)-ld
export RANLIB=$(TARGET)-ranlib
export AR=$(TARGET)-ar
export HOST=${TARGET}

SQUEEZEPLAY_SRC=${TOPDIR}/../squeezeplay/src


.PHONY: all s3c2412 system debug squeezeplay image clean distclean

all: system debug s3c2412 squeezeplay image


#
# System
#
system:
	cd system; ${MAKE} -f Makefile

debug:
	cd system; ${MAKE} -f Makefile debug


# Jive BSP
s3c2412: ${IMAGEDIR}
	cd s3c2412; ${MAKE} -f Makefile


#
# Squeezeplay
#
squeezeplay:
	cd ${SQUEEZEPLAY_SRC}; ${MAKE} -f Makefile.squeezeos TOPDIR=${TOPDIR}


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
	mknod ${STAGE}/dev/console c 5 1
	mknod ${STAGE}/dev/ttySAC0 c 204 64
	# Create cramfs
	mkcramfs ${STAGE} ${IMAGEDIR}/root.cramfs
	# Create upgrade.md5
	cd ${IMAGEDIR} && md5sum zImage-P7 root.cramfs > upgrade.md5
	# Create jive.bin
	VERSION=`cat ${SQUEEZEPLAY_SRC}/squeezeplay/src/version.h | cut -sd\" -f 2 | sed -e 's/ /_/'` && cd ${IMAGEDIR} && echo $$VERSION && zip ${IMAGEDIR}/jive_$$VERSION.bin jive.version upgrade.md5 zImage-P7 root.cramfs
	@cat ${IMAGEDIR}/jive.version


${IMAGEDIR}:
	-mkdir -p ${IMAGEDIR}

#
# Clean
#
clean:
	cd system; ${MAKE} -f Makefile clean
	cd s3c2412; ${MAKE} clean
	cd ../../squeezeplay/src; ${MAKE} -f Makefile.squeezeos TOPDIR=${TOPDIR} clean

distclean: clean
	rm -rf ${BUILDDIR}
