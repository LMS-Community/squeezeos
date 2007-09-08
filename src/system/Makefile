
# Absolute path to top of source directories
SRCDIR = $(realpath ${PWD}/..)
TOPDIR = $(dir ${SRCDIR})

export BUILDDIR=$(TOPDIR)build
export PREFIX=${BUILDDIR}/usr

export CFLAGS=-I${PREFIX}/include -Os
export LDFLAGS=-L${PREFIX}/lib

TARGET=arm-9tdmi-linux-gnu

export CC=$(TARGET)-gcc
export LD=$(TARGET)-ld
export RANLIB=$(TARGET)-ranlib
export AR=$(TARGET)-ar
export HOST=${TARGET}

LIB_DIR=$(TOPDIR)/toolchain/gcc-4.1.1-glibc-2.3.2/$(TARGET)/$(TARGET)/lib


.PHONY: all clean filesystem busybox strace oprofile popt bfd libiberty jivepowerd jivetest wireless-tools mtdutils zlib dropbear netperf jpeg png freetype expat openssl wavplay


#
# Top level targets
#
all: filesystem busybox jivepowerd jivetest wireless-tools mtdutils zlib dropbear netperf jpeg png freetype expat openssl wavplay


# debugging targets
debug: strace popt bfd libiberty oprofile
	install ${LIB_DIR}/libstdc++.* ${PREFIX}/lib
	install ${LIB_DIR}/libgcc_s.* ${PREFIX}/lib


# mtd-utils
mtdutils:
	cd mtd-utils; make RAWTARGETS="flash_eraseall nanddump nandwrite nandtest" DESTDIR=${BUILDDIR} install 


# filesystem
filesystem:
	cd filesystem; make install PREFIX=${BUILDDIR}


# busybox
busybox:
	cd busybox_1_7_stable; make oldconfig && make CROSS_COMPILE=$(TARGET)- busybox && make CROSS_COMPILE=$(TARGET)- install && cp -rf _install/bin _install/sbin _install/usr ${BUILDDIR}


# strace
strace-4.5.14/Makefile:
	cd strace-4.5.14; ./configure --host=${TARGET} --target=${TARGET} --prefix=/usr

strace: strace-4.5.14/Makefile
	cd strace-4.5.14; make; make DESTDIR=${BUILDDIR} install

# gdb
gdb-6.6/Makefile:
	cd gdb-6.6; ./configure --host=${TARGET} --prefix=/usr

gdb: gdb-6.6/Makefile
	cd gdb-6.6; make; make DESTDIR=${BUILDDIR} install


# oprofile
oprofile-0.9.2/Makefile:
	cd oprofile-0.9.2; ./configure --with-kernel-support --with-binutils=${PREFIX} --host=${TARGET} --target=${TARGET} --prefix=/usr

oprofile: oprofile-0.9.2/Makefile
	cd oprofile-0.9.2; make; make DESTDIR=${BUILDDIR} install


# popt
popt-1.7/Makefile:
	cd popt-1.7; ./configure --host=${TARGET} --target=${TARGET} --prefix=/usr

popt: popt-1.7/Makefile
	cd popt-1.7; make; make DESTDIR=${BUILDDIR} install


# binutils
binutils-2.17/bfd/Makefile:
	cd binutils-2.17/bfd; ./configure --host=${TARGET} --target=${TARGET} --prefix=/usr

bfd: binutils-2.17/bfd/Makefile
	cd binutils-2.17/bfd; make; make DESTDIR=${BUILDDIR} install

binutils-2.17/libiberty/Makefile:
	cd binutils-2.17/libiberty; ./configure --host=${TARGET} --target=${TARGET} --prefix=/usr

libiberty: binutils-2.17/libiberty/Makefile
	cd binutils-2.17/libiberty; make; make DESTDIR=${BUILDDIR} install


# jivetest
jivetest:
	cd jivetest; make; make install PREFIX=${PREFIX}


# wireless tools
wireless-tools:
	cd wireless_tools.28; make iwmulticall CC=${CC} PREFIX=${PREFIX}; make install-iwmulticall PREFIX=${PREFIX}


# zlib
zlib-1.2.3/Makefile:
	cd zlib-1.2.3; ./configure --shared --prefix=${PREFIX}

zlib: zlib-1.2.3/Makefile
	cd zlib-1.2.3; make; make install


# dropbear
dropbear-0.48.1/Makefile:
	cd dropbear-0.48.1; LD=${CC} ./configure --host=${TARGET} --target=${TARGET} --prefix=/usr

dropbear:dropbear-0.48.1/Makefile
	cd dropbear-0.48.1; make PROGRAMS="dropbear dropbearkey scp" ; make PROGRAMS="dropbear dropbearkey scp" install DESTDIR=${BUILDDIR}


# netperf	
netperf-2.4.3/Makefile:
	cd netperf-2.4.3 && ./configure --host=${TARGET} --target=${TARGET} --program-prefix= --prefix=/usr

netperf:netperf-2.4.3/Makefile
	cd netperf-2.4.3 && make && make install DESTDIR=${BUILDDIR}


# jpeg
jpeg-6b/Makefile:
	cd jpeg-6b; ./configure --enable-shared --host=${TARGET} --target=${TARGET} --prefix=${PREFIX}

jpeg: jpeg-6b/Makefile
	mkdir -p ${PREFIX}/man/man1
	cd jpeg-6b; make; make install; make install-lib; make install-headers


# png (requires zlib)
libpng-1.2.12/Makefile:
	cd libpng-1.2.12; ./configure --enable-shared --host=${TARGET} --target=${TARGET} --prefix=${PREFIX}

png: libpng-1.2.12/Makefile
	cd libpng-1.2.12; make; make DESTDIR= install


# freetype
freetype-2.1.10/config.mk:
	cd freetype-2.1.10; ./configure --host=${TARGET} --target=${TARGET} --prefix=${PREFIX} ${ENABLE_PROFILING}

freetype: freetype-2.1.10/config.mk
	cd freetype-2.1.10; make; make DESTDIR= install


# expat
expat-2.0.0/Makefile:
	cd expat-2.0.0; ./configure --enable-shared --host=${TARGET} --target=${TARGET} --prefix=/usr

.PHONY: expat
expat: expat-2.0.0/Makefile
	cd expat-2.0.0; make; make DESTDIR=${BUILDDIR} install


# opensll
openssl:
	cd openssl-0.9.7l; ./Configure --prefix=/usr no-rc5 no-idea linux-elf-arm; make CC=${CC} PERL=/usr/bin/perl; make install CC=${CC} PERL=/usr/bin/perl INSTALL_PREFIX=${BUILDDIR}


# wavplay
wavplay:
	cd wavplay-1.0; CPU= INSTDIR=${PREFIX}/bin make no_x install_no_x



.PHONY: clean
clean:
	-cd busybox_1_7_stable; make clean
	-cd strace-4.5.14; make distclean
	-cd oprofile-0.9.2; make distclean
	-cd popt-1.7; make distclean
	-cd binutils-2.17/bfd; make distclean
	-cd binutils-2.17/libiberty; make distclean
	-cd jivetest; make clean
	-cd wireless_tools.28; make clean
	-cd mtd-utils; make clean
	-cd zlib-1.2.3; make distclean; rm Makefile
	-cd dropbear-0.48.1; make distclean
	-cd netperf-2.4.3; make distclean
	-cd jpeg-6b; make distclean
	-cd libpng-1.2.12; make distclean
	-cd freetype-2.1.10; make distclean
	-cd expat-2.0.0; make distclean
	-cd openssl-0.9.7l; make clean
	-cd wavplay-1.0; make real_clean
