DESCRIPTION = "SqueezeOS - base image with debugging"

require squeezeos-image.bb

IMAGE_INSTALL += "squeezeplay-dbg"
IMAGE_INSTALL += "gdb gdbserver"
IMAGE_INSTALL += "strace"

# oprofile build not working with csl2009q3 toolchain
# IMAGE_INSTALL += "oprofile"
