DESCRIPTION = "WavPack Libraries"
SECTION = "libs"
LICENSE = "LGPL"

PR="r0"

SRC_URI = "http://www.mega-nerd.com/SRC/${PN}-${PV}.tar.gz"

S="${WORKDIR}/${PN}-${PV}"

inherit autotools

EXTRA_OECONF = "--enable-static"

do_stage() {
	autotools_stage_all
}

