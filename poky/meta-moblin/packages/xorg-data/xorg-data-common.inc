HOMEPAGE = "http://www.x.org"
SECTION = "x11/data"
LICENSE = "MIT-X"

XORG_PN = "${PN}"

SRC_URI = "${XORG_MIRROR}/individual/data/${XORG_PN}-${PV}.tar.bz2"
S = "${WORKDIR}/${XORG_PN}-${PV}"

inherit autotools_stage pkgconfig
