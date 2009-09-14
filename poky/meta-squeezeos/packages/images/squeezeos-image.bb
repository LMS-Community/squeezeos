DESCRIPTION = "SqueezeOS - base image"

require squeezeos-image-boot.bb

IMAGE_INSTALL += "squeezeplay"
IMAGE_INSTALL += "dropbear watchdog"

# wireless tools
IMAGE_INSTALL += "wireless-tools"

# useful debug tools
IMAGE_INSTALL += "stress"
#IMAGE_INSTALL += "tcpdump"

# useful command line tools
IMAGE_INSTALL += "patch procps alsa-utils-aplay alsa-utils-amixer"

# Simple NTP client
IMAGE_INSTALL += "msntp"
