IMAGE_INSTALL = "task-poky-boot"

IMAGE_LINGUAS = " "

inherit poky-image

# remove not needed ipkg informations
ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files"

IMAGE_INSTALL += "squeezeplay dropbear"
