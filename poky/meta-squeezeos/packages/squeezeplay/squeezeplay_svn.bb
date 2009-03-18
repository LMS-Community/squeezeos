DESCRIPTION = "SqueezePlay"
LICENSE = "Logitech Public Source License"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r9"

DEPENDS += "libsdl libsdl-ttf libsdl-gfx libsdl-image pango libsdl-pango"
DEPENDS += "lua lua-native luatolua++"
DEPENDS += "flac libmad tremor"

RDEPENDS += "liblua5.1-socket liblua5.1-json liblua5.1-zipfilter liblua5.1-loop liblua5.1-logging liblua5.1-syslog liblua5.1-filesystem liblua5.1-profiler liblua5.1-tolua++ liblua5.1-md5"

RDEPENDS += "pango-module-arabic-fc pango-module-hangul-fc pango-module-hebrew-fc pango-module-indic-fc pango-module-khmer-fc pango-module-syriac-fc pango-module-thai-fc pango-module-tibetan-fc"

RDEPENDS += "freefont"

SRC_URI = "${SQUEEZEPLAY_SCM};module=squeezeplay"

S = "${WORKDIR}/squeezeplay"

inherit autotools

EXTRA_OECONF = "--disable-portaudio"

# Optional close source package
DEPENDS += "${@base_conditional('ENABLE_SPPRIVATE', 'yes', 'squeezeplay-private', '', d)}"
EXTRA_OECONF += "${@base_conditional('ENABLE_SPPRIVATE', 'yes', '--with-spprivate', '', d)}"

CFLAGS_prepend = '-DSQUEEZEPLAY_RELEASE=\\"${DISTRO_VERSION}\\" -DSQUEEZEPLAY_REVISION=\\"${SQUEEZEOS_REVISION}\\"'

CFLAGS += -I${STAGING_INCDIR}/glib-2.0 -I${STAGING_INCDIR}/../lib/glib-2.0/include -I${STAGING_INCDIR}/pango-1.0 -I${STAGING_INCDIR}/freetype2 -I${STAGING_INCDIR}/cairo


EXTRA_OEMAKE = "all lua-lint"


do_stage() {
	install -d ${STAGING_INCDIR}/squeezeplay
	install -d ${STAGING_INCDIR}/squeezeplay/ui
	install -d ${STAGING_INCDIR}/squeezeplay/audio
	install -m 0644 src/debug.h ${STAGING_INCDIR}/squeezeplay/debug.h
	install -m 0644 src/types.h ${STAGING_INCDIR}/squeezeplay/types.h
	install -m 0644 src/ui/jive.h ${STAGING_INCDIR}/squeezeplay/ui/jive.h
	install -m 0644 src/audio/fifo.h ${STAGING_INCDIR}/squeezeplay/audio/fifo.h
	install -m 0644 src/audio/streambuf.h ${STAGING_INCDIR}/squeezeplay/audio/streambuf.h
}

FILES_${PN} += "${datadir}"
