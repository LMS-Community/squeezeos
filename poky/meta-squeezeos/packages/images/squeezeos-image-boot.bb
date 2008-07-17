DESCRIPTION = "SqueezeOS - minimal bootable image"
PACKAGE_ARCH = "${MACHINE_ARCH}"
DEPENDS = "virtual/kernel"
PR = "r1"

inherit image squeezeos-upgrade-image

IMAGE_INSTALL = " \
	squeezeos-base-files \
	busybox \
	mtd-utils \
	ubi-utils"

IMAGE_LINGUAS = " "

# remove not needed ipkg informations
ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files"
