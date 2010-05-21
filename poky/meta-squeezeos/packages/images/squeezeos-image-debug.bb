DESCRIPTION = "SqueezeOS - base image with debugging"

require squeezeos-image.bb

IMAGE_INSTALL += "squeezeplay-dbg"
IMAGE_INSTALL += "gdb gdbserver"
IMAGE_INSTALL += "strace"
IMAGE_INSTALL += "oprofile"

