#!/bin/bash
#
# Simple script to collect/generate the kernel sources for building a RT-kernel
# for the fab4. The structure is patch based to make it easier to integrate
# future releases of parts of this kernel, such as updates from Freescale, 
# RT-patch, UBI-fs, and even the base kernel itself. Patches make clear what
# our custom modifications are and make them easier reviewable by others.
#
# Author: Remy Bohmer <linux@bohmer.net>
#

CURDIR=$(pwd)
POKY_SRCS_DIR=${CURDIR}/../../../poky/sources
KERNEL_BASE=linux-2.6.24
KERNEL_SRC=${KERNEL_BASE}.tar.bz2
BUILD_DIR=${CURDIR}/build
DOWNLOAD_DIR=${BUILD_DIR}/srcs
KERNEL_ARCHIVE=${DOWNLOAD_DIR}/${KERNEL_SRC}

function die() {
	echo "$1"
	exit 1
}

# Check current location
if [ "$(basename $(pwd))" != "linux-2.6.24-rt" ]; then
	die "Please, execute this script from the linux-2.6.24-rt directory"
fi

if [ -d ${BUILD_DIR}/${KERNEL_BASE} ]; then
	die "A kernel tree already exists at ${BUILD_DIR}/${KERNEL_BASE}, please remove it first"
fi

mkdir -p ${DOWNLOAD_DIR} || die "Failed to create temp directories"

echo "1. Get the kernel sources..."
if [ -d ${POKY_SRCS_DIR} ]; then
	if [ -f ${POKY_SRCS_DIR}/${KERNEL_SRC} ]; then
		KERNEL_ARCHIVE=${POKY_SRCS_DIR}/${KERNEL_SRC}
	fi
fi
if [ "x${KERNEL_SRCS}" = "x" -a ! -f ${DOWNLOAD_DIR}/${KERNEL_SRC} ]; then
	(cd ${DOWNLOAD_DIR} && wget http://www.kernel.org/pub/linux/kernel/v2.6/${KERNEL_SRC}) || die "Failed to download kernel sources"
fi

echo "2. Extract the kernel sources..."
(cd ${BUILD_DIR} && tar xjf ${KERNEL_ARCHIVE}) || die "Failed to extract kernel sources"

echo "3. Apply patches..."
(cd ${BUILD_DIR}/${KERNEL_BASE} && ln -s ../../patches && ln -s patches/series)
(cd ${BUILD_DIR}/${KERNEL_BASE} && quilt push -a) || "Failed to apply kernel patches"

echo ""
echo "===================================================="
echo "The RT-kernel is now ready to be build/used..."
echo ""
echo "'quilt' can be used to maintain the stack of patches"
echo "from within the directory:"
echo "${BUILD_DIR}/${KERNEL_BASE}"
echo "===================================================="

