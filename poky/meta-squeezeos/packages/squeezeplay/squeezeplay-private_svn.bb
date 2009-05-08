DESCRIPTION = "SqueezePlay - Private code"
LICENSE = "Confidential"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
#PR = "r0"

DEPENDS += "libsdl libsdl-ttf libsdl-gfx libsdl-image"
RDEPENDS += "libsdl libsdl-ttf libsdl-gfx libsdl-image"

DEPENDS += "lua lua-native luatolua++"
RDEPENDS += "liblua5.1-socket liblua5.1-json liblua5.1-zipfilter liblua5.1-loop liblua5.1-logging liblua5.1-syslog liblua5.1-filesystem liblua5.1-profiler liblua5.1-tolua++ liblua5.1-md5"

DEPENDS += "flac libmad tremor"

RDEPENDS += "freefont"


DEPENDS += "axtls"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=squeezeplay_private"

S = "${WORKDIR}/squeezeplay_private"

inherit autotools

EXTRA_OECONF = "--disable-shared"

do_stage() {
	autotools_stage_all
}

# Just build a shared library, don't install any packages
FILES_${PN} = ""
FILES_${PN}-dbg = ""
