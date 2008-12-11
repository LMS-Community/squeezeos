DESCRIPTION = "SqueezeOS - debugging tools"

inherit image

IMAGE_LINGUAS = " "

IMAGE_INSTALL += "strace oprofile gdbserver"

IMAGE_FSTYPES = "tar.gz"

# remove not needed ipkg informations
ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files"
