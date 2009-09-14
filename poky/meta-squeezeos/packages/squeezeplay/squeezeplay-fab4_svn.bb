DESCRIPTION = "SqueezePlay - Fab4 specific code"
LICENSE = "Logitech Public Source License"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r2"

DEPENDS += "squeezeplay"

SRC_URI="${SQUEEZEPLAY_SCM};module=squeezeplay_fab4"

S = "${WORKDIR}/squeezeplay_fab4"

inherit autotools

CFLAGS_prepend = '-I${STAGING_INCDIR}/squeezeplay'

EXTRA_OEMAKE = "all lua-lint"

# Optional close source package
DEPENDS += "${@base_conditional('ENABLE_SPPRIVATE', 'yes', 'squeezeplay-private', '', d)}"
EXTRA_OECONF += "${@base_conditional('ENABLE_SPPRIVATE', 'yes', '--with-spprivate', '', d)}"


do_install() {
	autotools_do_install

	# move lua libraries to correct location
	rm ${D}${libdir}/* 
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 .libs/fab4_bsp.so ${D}${libdir}/lua/5.1/fab4_bsp.so
}


FILES_${PN} += "${datadir} ${libdir}/lua/5.1"
FILES_${PN}-dbg += "${libdir}/lua/5.1/.debug"
