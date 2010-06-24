DESCRIPTION = "SqueezePlay"
LICENSE = "Logitech Public Source License"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r24"

DEPENDS += "libsdl libsdl-ttf libsdl-gfx libsdl-image"
DEPENDS += "lua lua-native luatolua++"
DEPENDS += "flac libmad tremor"

RDEPENDS += "liblua5.1-socket liblua5.1-json liblua5.1-zipfilter liblua5.1-loop liblua5.1-filesystem liblua5.1-profiler liblua5.1-tolua++ liblua5.1-md5 liblua5.1-expat"
RDEPENDS += "freefont"

SRC_URI = "${SQUEEZEPLAY_SCM};module=squeezeplay \
	file://logconf.lua"

S = "${WORKDIR}/squeezeplay"

ARM_INSTRUCTION_SET = "arm"

inherit autotools

EXTRA_OECONF = "--disable-portaudio --enable-fsync-workaround"

EXTRA_OECONF_append_baby = " --enable-screen-rotation"

# Optional close source package
DEPENDS += "${@base_conditional('ENABLE_SPPRIVATE', 'yes', 'squeezeplay-private', '', d)}"
EXTRA_OECONF += "${@base_conditional('ENABLE_SPPRIVATE', 'yes', '--with-spprivate', '', d)}"

CFLAGS_prepend = '-DSQUEEZEPLAY_RELEASE=\\"${DISTRO_RELEASE}\\" -DSQUEEZEPLAY_REVISION=\\"${SQUEEZEOS_REVISION}\\"'

EXTRA_OEMAKE = "all lua-lint"


do_stage() {
	install -d ${STAGING_INCDIR}/squeezeplay
	install -d ${STAGING_INCDIR}/squeezeplay/ui
	install -d ${STAGING_INCDIR}/squeezeplay/audio
	install -m 0644 src/log.h ${STAGING_INCDIR}/squeezeplay/log.h
	install -m 0644 src/types.h ${STAGING_INCDIR}/squeezeplay/types.h
	install -m 0644 src/ui/jive.h ${STAGING_INCDIR}/squeezeplay/ui/jive.h
	install -m 0644 src/audio/fifo.h ${STAGING_INCDIR}/squeezeplay/audio/fifo.h
	install -m 0644 src/audio/streambuf.h ${STAGING_INCDIR}/squeezeplay/audio/streambuf.h
}

do_install_append() {
	install -m 0644 ${WORKDIR}/logconf.lua ${D}${datadir}/jive/logconf.lua

}

PACKAGES = "${PN}-dbg ${PN}-qvgaskin ${PN}-jiveskin ${PN}-fab4skin ${PN}-babyskin ${PN}"

FILES_${PN}-qvgaskin += "\
	${datadir}/jive/applets/QVGAbaseSkin \
"

FILES_${PN}-jiveskin += "\
	${datadir}/jive/applets/QVGAportraitSkin \
"

FILES_${PN}-babyskin += "\
	${datadir}/jive/applets/QVGAlandscapeSkin \
"

FILES_${PN}-fab4skin += "\
	${datadir}/jive/applets/WQVGAlargeSkin \
	${datadir}/jive/applets/WQVGAsmallSkin \
"

FILES_${PN} += "${datadir}"
