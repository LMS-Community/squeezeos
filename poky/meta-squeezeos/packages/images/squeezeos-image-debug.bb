DESCRIPTION = "SqueezeOS - base image with debugging"

require squeezeos-image-boot.bb

IMAGE_INSTALL += "squeezeplay-dbg dropbear patch wireless-tools alsa-utils-aplay alsa-utils-amixer"
