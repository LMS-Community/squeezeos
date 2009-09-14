DESCRIPTION = "SqueezePlay - Baby specific code"
LICENSE = "Logitech Public Source License"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r1"

DEPENDS += "squeezeplay"

SRC_URI="${SQUEEZEPLAY_SCM};module=squeezeplay_baby"

S = "${WORKDIR}/squeezeplay_baby"

inherit autotools

CFLAGS_prepend = '-I${STAGING_INCDIR}/squeezeplay'

EXTRA_OEMAKE = "all lua-lint"


do_install() {
	autotools_do_install

	# move lua libraries to correct location
	rm ${D}${libdir}/* 
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 .libs/baby_bsp.so ${D}${libdir}/lua/5.1/baby_bsp.so
}


FILES_${PN} += "${datadir} ${libdir}/lua/5.1"
FILES_${PN}-dbg += "${libdir}/lua/5.1/.debug"
