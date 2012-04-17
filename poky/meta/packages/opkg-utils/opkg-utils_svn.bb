DESCRIPTION = "OPKG Package Manager Utilities"
SECTION = "base"
PRIORITY = "optional"
LICENSE = "GPL"
RDEPENDS = "python"
PR = "r2"

SRC_URI = "http://downloads.slimdevices.com/poky-cache/opkg-utils_svn.openmoko.org_.trunk.src.host._4534_.tar.gz"

S = "${WORKDIR}/opkg-utils"

inherit autotools_stage

S = "${WORKDIR}/opkg-utils"
