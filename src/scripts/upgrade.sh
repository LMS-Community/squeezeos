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
DISP=/tmp


# Need to copy programs into /tmp out of the cramfs
cp /usr/disp/* $DISP
cp /bin/busybox /tmp
cp /usr/sbin/flash_eraseall_y /tmp
cp /usr/sbin/nandwrite_y /tmp
cp /usr/sbin/nanddump_y /tmp


FLASH_ERASE_ALL=/tmp/flash_eraseall_y
NANDWRITE=/tmp/nandwrite_y
NANDDUMP=/tmp/nanddump_y
GREP='/tmp/busybox grep'
ECHO='/tmp/busybox echo'



# Check SD card contents
FAIL=0

if [ ! -r ${MMC}/u-boot.bin ] ; then
    echo "ERROR: missing u-boot.bin"
    ${DISP}/disp_uboot_nofile
    FAIL=1
fi

if [ ! -r ${MMC}/zImage ] ; then
    echo "ERROR: missing zImage"
    ${DISP}/disp_kernel_nofile
    FAIL=1
fi

if [ ! -r ${MMC}/root.cramfs ] ; then
    echo "ERROR: missing roof.cramfs"
    ${DISP}/disp_rootfs_nofile
    FAIL=1
fi

if [ ! -r ${MMC}/upgrade.md5 ] ; then
    echo "ERROR: missing upgrade.md5"
    ${DISP}/disp_md5_nofile
    FAIL=1
fi

if [ $FAIL == 1 ] ; then
    exit 1
fi


# Check flash filesystem
BOOTLDMTD=`cat /proc/mtd | grep -i Bootloader | cut -c4`
KERNELMTD=`cat /proc/mtd | grep -i 'Kernel A' | cut -c4`
ROOTFSMTD=`cat /proc/mtd | grep -i 'NAND Root(UI) A' | cut -c4`

# For testing
#BOOTLDMTD=`cat /proc/mtd | grep -i Yaffs | cut -c4`
#KERNELMTD=`cat /proc/mtd | grep -i 'Kernel B' | cut -c4`
#ROOTFSMTD=`cat /proc/mtd | grep -i 'NAND Root(UI) B' | cut -c4`

echo "boot mtd check"
if [ "x$BOOTLDMTD" == "x" -o ! -r /dev/mtd/$BOOTLDMTD ] ; then
    echo "ERROR: unable to find uboot mtd"
    ${DISP}/disp_uboot_nofile
    exit 1
fi

echo "kernel mtd check"
if [ "x$KERNELMTD" == "x" -o ! -r /dev/mtd/$KERNELMTD ] ; then
    echo "ERROR: unable to find kernel mtd"
    ${DISP}/disp_kernel_nofile
    exit 1
fi

echo "rootfs mtd check"
if [ "x$ROOTFSMTD" == "x" -o ! -r /dev/mtd/$ROOTFSMTD ] ; then
    echo "ERROR: unable to find rootfs mtd"
    ${DISP}/disp_rootfs_nofile
    exit 1
fi


# Copy files
echo "copy uboot.bin to ram"
${DISP}/disp_uboot_copy
cp ${MMC}/u-boot.bin ${TMP}/u-boot.bin
${DISP}/disp_uboot_copy_end	

echo "copy zImage to ram"
${DISP}/disp_kernel_copy
cp ${MMC}/zImage ${TMP}/zImage
${DISP}/disp_kernel_copy_end

echo "copy root.cramfs to ram"
${DISP}/disp_rootfs_copy
cp ${MMC}/root.cramfs ${TMP}/root.cramfs
${DISP}/disp_rootfs_copy_end

echo "copy upgrade.md5 to ram"
cp ${MMC}/upgrade.md5 ${TMP}/upgrade.md5

BOOTLDSZ=`ls -l ${TMP}/u-boot.bin | awk -F" " '{print $5}'`
KERNELSZ=`ls -l ${TMP}/zImage | awk -F" " '{print $5}'`
ROOTFSSZ=`ls -l ${TMP}/root.cramfs | awk -F" " '{print $5}'`


# All ready
echo "BOOTLDMTD=/dev/mtd/$BOOTLDMTD SIZE=${BOOTLDSZ}"
echo "KERNELMTD=/dev/mtd/$KERNELMTD SIZE=${KERNELSZ}"
echo "ROOTFSMTD=/dev/mtd/$ROOTFSMTD SIZE=${ROOTFSSZ}"
echo "OK: starting upgrade"


#
# From this stage don't access the cramfs
#



# Kernel
${ECHO} "${FLASH_ERASE_ALL} /dev/mtd/$KERNELMTD"
${FLASH_ERASE_ALL} /dev/mtd/$KERNELMTD
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: flash_eraseall kernel failed"
    ${DISP}/disp_kernel_erase_fail
    exit 1
fi

${ECHO} "${NANDWRITE} -qp /dev/mtd/$KERNELMTD ${TMP}/zImage"
${NANDWRITE} -qp /dev/mtd/$KERNELMTD ${TMP}/zImage 
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: nandwrite kernel failed"
    ${DISP}/disp_kernel_write_fail
    exit 1
fi

${ECHO} "CKSUM=${NANDDUMP} -obl ${KERNELSZ} /dev/mtd/$KERNELMTD | md5sum | cut -d- -f1"
CKSUM=`${NANDDUMP} -obl ${KERNELSZ} /dev/mtd/$KERNELMTD | md5sum | cut -d- -f1`
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: nanddump kernel failed"
    ${DISP}/disp_kernel_dump_fail
    exit 1
fi

${ECHO} "KERNEL CKSUM=${CKSUM}"
${GREP} ${CKSUM} ${TMP}/upgrade.md5
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: kernel checksum failed"
    ${DISP}/disp_kernel_checksum_fail
    exit 1
else
    ${ECHO} "OK: kernel checksum"
    ${DISP}/disp_kernel_write_success
fi



# Bootloader
# Do the bootloader second, we've proven writing to the flash works by now, so we have
# less risk of bricking the board here.

${ECHO} "${FLASH_ERASE_ALL} /dev/mtd/$BOOTLDMTD"
${FLASH_ERASE_ALL} /dev/mtd/$BOOTLDMTD
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: flash_eraseall uboot failed"
    ${DISP}/disp_uboot_erase_fail
    exit 1
fi

${ECHO} "${NANDWRITE} -qp /dev/mtd/$BOOTLDMTD ${TMP}/u-boot.bin"
${NANDWRITE} -qp /dev/mtd/$BOOTLDMTD ${TMP}/u-boot.bin 
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: nandwrite uboot failed"
    ${DISP}/disp_uboot_write_fail
    exit 1
fi		

${ECHO} "CKSUM=${NANDDUMP} -obl ${BOOTLDSZ} /dev/mtd/${BOOTLDMTD} | md5sum | cut -d- -f1"
CKSUM=`${NANDDUMP} -obl ${BOOTLDSZ} /dev/mtd/${BOOTLDMTD} | md5sum | cut -d- -f1`
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: nanddump bootloader failed"
    exit 1
fi

${ECHO} "UBOOT CKSUM=${CKSUM}"
${GREP} ${CKSUM} ${TMP}/upgrade.md5
if [ $? != "0" ] ; then
    ${DISP}/disp_uboot_checksum_fail
    ${ECHO} "ERROR: uboot checksum failed"
    exit 1
else
    ${DISP}/disp_uboot_write_success
    ${ECHO} "OK: uboot checksum success"
fi
${ECHO} "OK: uboot"



# Filesystem
${ECHO} "${FLASH_ERASE_ALL} /dev/mtd/$ROOTFSMTD"
${FLASH_ERASE_ALL} /dev/mtd/$ROOTFSMTD
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: flash_eraseall rootfs failed"
    ${DISP}/disp_rootfs_erase_fail
    exit 1
fi

${ECHO} "${NANDWRITE} -qp /dev/mtd/$ROOTFSMTD ${TMP}/root.cramfs"
${NANDWRITE} -qp /dev/mtd/$ROOTFSMTD ${TMP}/root.cramfs
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: nandwrite rootfs failed"
    ${DISP}/disp_rootfs_write_fail
    exit 1
fi

${ECHO} "CKSUM=${NANDDUMP} -obl ${ROOTFSSZ} /dev/mtd/$ROOTFSMTD | md5sum | cut -d- -f1"
CKSUM=`${NANDDUMP} -obl ${ROOTFSSZ} /dev/mtd/$ROOTFSMTD | md5sum | cut -d- -f1`
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: nanddump rootfs failed"
    ${DISP}/disp_rootfs_dump_fail
    exit 1
fi

${ECHO} "ROOTFS CKSUM=${CKSUM}"
${GREP} ${CKSUM} ${TMP}/upgrade.md5
if [ $? != "0" ] ; then
    ${ECHO} "ERROR: rootfs checksum failed"
    ${DISP}/disp_rootfs_checksum_fail
    exit 1
else
    ${ECHO} "OK: rootfs checksum"
    ${DISP}/disp_rootfs_write_success
fi			



# We are done
${ECHO} "OK: upgrade completed"
exit 0
