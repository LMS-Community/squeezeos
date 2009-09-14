DESCRIPTION = "SqueezeOS - SqueezeCenter"

require squeezeos-image.bb

IMAGE_LINGUAS = " "

IMAGE_INSTALL += "squeezecenter"

#IMAGE_FSTYPES = "tar.gz"

# remove not needed ipkg informations
ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files"
