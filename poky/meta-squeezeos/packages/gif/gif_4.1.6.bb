DESCRIPTION = "GIF Library"
HOMEPAGE = "http://sourceforge.net/projects/giflib/"
LICENSE = "giflib"
SECTION = "libs"
PR = "r1"

SRC_URI = "${SOURCEFORGE_MIRROR}/giflib/giflib-${PV}.tar.bz2"

ARM_INSTRUCTION_SET = "arm"

inherit autotools

do_stage() {
	autotools_stage_all
}

PACKAGES += "giflib"
FILES_${PN} = "${bindir}/*"
FILES_giflib = "${libdir}/libgif.so.*"

