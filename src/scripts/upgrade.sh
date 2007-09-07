#!/bin/sh

# WARNING:
# This script must only be used for the initial factory upgrade

# The SD card should include:
#  upgrade.sh    -- this script
#  zImage        -- the kernel image
#  root.cramfs   -- the filesystem
#  upgrade.md5   -- md5sum of zImage root.cramfs


MMC=/mnt/mmc
TMP=/tmp

FLASH_ERASE_ALL=/usr/sbin/flash_eraseall
NANDWRITE=/usr/sbin/nandwrite
NANDDUMP=/usr/sbin/nanddump


# Check SD card contents
if [ ! -r ${MMC}/zImage ] ; then
    echo "ERROR: missing zImage"
    exit 1
fi

if [ ! -r ${MMC}/root.cramfs ] ; then
    echo "ERROR: missing roof.cramfs"
    exit 1
fi

if [ ! -r ${MMC}/upgrade.md5 ] ; then
    echo "ERROR: missing upgrade.md5"
    exit 1
fi


# Check flash filesystem
BOOTLDMTD=`cat /proc/mtd | grep -i Bootloader | cut -c4`
KERNELMTD=`cat /proc/mtd | grep -i Kernel | cut -c4`
ROOTFSMTD=`cat /proc/mtd | grep -i Root | cut -c4`

if [ "x" = "x${BOOTLDMTD}" ] ; then
    echo "ERROR: unable to find uboot mtd"
    exit 1
fi

if [ "x" = "x${KERNELMTD}" ] ; then
    echo "ERROR: unable to find kernel mtd"
    exit 1
fi

if [ "x" = "x${ROOTFSMTD}" ] ; then
    echo "ERROR: unable to find rootfs mtd"
    exit 1
fi


# Copy files
cp ${MMC}/zImage ${TMP}/zImage
cp ${MMC}/root.cramfs ${TMP}/root.cramfs
cp ${MMC}/upgrade.md5 ${TMP}/upgrade.md5

KERNELSZ=`ls -l ${TMP}/zImage | awk -F" " '{print $5}'`
ROOTFSSZ=`ls -l ${TMP}/root.cramfs | awk -F" " '{print $5}'`


# All ready
echo "BOOTLDMTD=${BOOTLDMTD}"
echo "KERNELMTD=${KERNELMTD} SIZE=${KERNELSZ}"
echo "ROOTFSMTD=${ROOTFSMTD} SIZE=${ROOTFSSZ}"
echo "OK: starting upgrade"


# Bootloader
## we don't need to upgrade this now


# Kernel
${FLASH_ERASE_ALL} /dev/mtd/${KERNELMTD}
if [ $? != "0" ] ; then
    echo "ERROR: flash_eraseall kernel failed"
    exit 1
fi

${NANDWRITE} -qp /dev/mtd/${KERNELMTD} ${TMP}/zImage 
if [ $? != "0" ] ; then
    echo "ERROR: nandwrite kernel failed"
    exit 1
fi

CKSUM=`${NANDDUMP} -obl ${KERNELSZ} /dev/mtd/${KERNELMTD} | md5sum | cut -d- -f1`
if [ $? != "0" ] ; then
    echo "ERROR: nanddump kernel failed"
    exit 1
fi

echo "KERNEL CKSUM=${CKSUM}"
grep ${CKSUM} ${TMP}/upgrade.md5
if [ $? != "0" ] ; then
    echo "ERROR: kernel checksum failed"
    exit 1
fi
echo "OK: kernel checksum"


# Filesystem
${FLASH_ERASE_ALL} /dev/mtd/${ROOTFSMTD}
if [ $? != "0" ] ; then
    echo "ERROR: flash_eraseall rootfs failed"
    exit 1
fi

${NANDWRITE} -qp /dev/mtd/${ROOTFSMTD} ${TMP}/root.cramfs
if [ $? != "0" ] ; then
    echo "ERROR: nandwrite rootfs failed"
    exit 1
fi

CKSUM=`${NANDDUMP} -obl ${ROOTFSSZ} /dev/mtd/${ROOTFSMTD} | md5sum | cut -d- -f1`
if [ $? != "0" ] ; then
    echo "ERROR: nanddump rootfs failed"
    exit 1
fi

echo "ROOTFS CKSUM=${CKSUM}"
grep ${CKSUM} ${TMP}/upgrade.md5
if [ $? != "0" ] ; then
    echo "ERROR: rootfs checksum failed"
    exit 1
fi
echo "OK: rootfs checksum"


# We are done
echo "OK: upgrade completed"
exit 0
