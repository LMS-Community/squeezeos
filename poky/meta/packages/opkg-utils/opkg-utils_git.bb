DESCRIPTION = "OPKG Package Manager Utilities"
SECTION = "base"
PRIORITY = "optional"
LICENSE = "GPL"
RDEPENDS = "python"
PR = "r2"

SRC_URI = "git://git.yoctoproject.org/opkg-utils;protocol=git"

S = "${WORKDIR}/git"

inherit autotools_stage

S = "${WORKDIR}/git"
