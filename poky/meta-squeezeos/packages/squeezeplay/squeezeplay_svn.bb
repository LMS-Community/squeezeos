DESCRIPTION = "SqueezePlay"
LICENSE = "Logitech Public Source License"

PV = "7.2+svnr${SRCREV}"
PR = "r2"

DEPENDS += "libsdl libsdl-ttf libsdl-gfx libsdl-image"
RDEPENDS += "libsdl libsdl-ttf libsdl-gfx libsdl-image"

DEPENDS += "lua luatolua++"
RDEPENDS += "liblua5.1-socket liblua5.1-json liblua5.1-zipfilter liblua5.1-loop liblua5.1-logging liblua5.1-syslog liblua5.1-filesystem liblua5.1-profiler liblua5.1-tolua++"

# FIXME should be libflac but this does not work?
DEPENDS += "portaudio flac libmad tremor"

RDEPENDS += "freefont"

SRC_URI="${SQUEEZEPLAY_SCM};module=squeezeplay"

S = "${WORKDIR}/squeezeplay"

inherit autotools

CFLAGS_prepend = '-DSQUEEZEPLAY_RELEASE=\\"${DISTRO_VERSION}\\" -DSQUEEZEPLAY_REVISION=\\"${SQUEEZEOS_REVISION}\\"'

FILES_${PN} += "${datadir}"
