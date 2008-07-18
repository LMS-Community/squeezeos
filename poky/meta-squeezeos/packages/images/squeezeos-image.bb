DESCRIPTION = "SqueezeOS - base image"

require squeezeos-image-boot.bb

IMAGE_INSTALL += "squeezeplay dropbear patch wireless-tools alsa-utils-aplay"
