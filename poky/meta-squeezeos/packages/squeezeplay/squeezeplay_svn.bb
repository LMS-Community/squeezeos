DESCRIPTION = "SqueezePlay"
LICENSE = "Logitech Public Source License"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r9"

DEPENDS += "libsdl libsdl-ttf libsdl-gfx libsdl-image pango libsdl-pango"
DEPENDS += "lua lua-native luatolua++"
DEPENDS += "flac libmad tremor"

RDEPENDS += "liblua5.1-socket liblua5.1-json liblua5.1-zipfilter liblua5.1-loop liblua5.1-logging liblua5.1-syslog liblua5.1-filesystem liblua5.1-profiler liblua5.1-tolua++"

RDEPENDS += "pango-module-arabic-fc pango-module-hangul-fc pango-module-hebrew-fc pango-module-indic-fc pango-module-khmer-fc pango-module-syriac-fc pango-module-thai-fc pango-module-tibetan-fc"

RDEPENDS += "freefont"

SRC_URI = "${SQUEEZEPLAY_SCM};module=squeezeplay"

S = "${WORKDIR}/squeezeplay"

inherit autotools

EXTRA_OECONF = "--disable-portaudio"

CFLAGS_prepend = '-DSQUEEZEPLAY_RELEASE=\\"${DISTRO_VERSION}\\" -DSQUEEZEPLAY_REVISION=\\"${SQUEEZEOS_REVISION}\\"'

CFLAGS += -I${STAGING_INCDIR}/glib-2.0 -I${STAGING_INCDIR}/../lib/glib-2.0/include -I${STAGING_INCDIR}/pango-1.0 -I${STAGING_INCDIR}/freetype2 -I${STAGING_INCDIR}/cairo


EXTRA_OEMAKE = "all lua-lint"


do_stage() {
	install -m 0644 src/ui/jive.h ${STAGING_INCDIR}/
}

FILES_${PN} += "${datadir}"
