DESCRIPTION = "Library for reading some sort of media format."
SECTION = "libs"
LICENSE = "LGPL"

PR="r0"

#SRC_URI = "${SOURCEFORGE_MIRROR}/faac/${PN}-${PV}.tar.gz"

SRC_URI = "${SOURCEFORGE_MIRROR}/faac/${PN}-${PV}.tar.gz \
           file://sc.patch;patch=1 \
           file://bpa-stdin.patch;patch=1"

S="${WORKDIR}/${PN}-${PV}"

inherit autotools

EXTRA_OECONF = "--without-xmms --without-drm --without-mpeg4ip --disable-shared"

FILES_${PN} = "${bindir}/faad"
