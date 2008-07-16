DESCRIPTION = "SqueezeOS - minimal bootable image"

IMAGE_LINGUAS = " "

inherit image

IMAGE_INSTALL = "task-squeezeos-base"
IMAGE_INSTALL += "mtd-utils ubi-utils"

# remove not needed ipkg informations
ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files"
