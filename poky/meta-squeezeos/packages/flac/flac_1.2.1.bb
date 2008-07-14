DESCRIPTION = "FLAC"
SECTION = "libs"
LICENSE = "Xiph"

PR = "r1"

SRC_URI="${SOURCEFORGE_MIRROR}/flac/flac-${PV}.tar.gz"

S = "${WORKDIR}/flac-${PV}"

inherit autotools

EXTRA_OECONF = "--disable-ogg --disable-xmms-plugin"

do_stage() {
	autotools_stage_all
}

