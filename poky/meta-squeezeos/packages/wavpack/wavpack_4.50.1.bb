DESCRIPTION = "WavPack Libraries"
SECTION = "libs"
LICENSE = "LGPL"

PR="r0"

SRC_URI = "http://www.wavpack.com/${PN}-${PV}.tar.bz2"

S="${WORKDIR}/${PN}-${PV}"

inherit autotools

EXTRA_OECONF = "--enable-static"

do_stage() {
	autotools_stage_all
}

