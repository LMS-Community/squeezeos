DESCRIPTION = "Stand Alone SqueezeCenter Package with everything needed"

inherit image

IMAGE_INSTALL += "squeezecenter faad2 alac flac"

IMAGE_LINGUAS = " "

IMAGE_FSTYPES = "tar.gz"

# remove not needed ipkg informations
ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files"


