DESCRIPTION = "SqueezePlay - SqueezeOS specific code"
LICENSE = "Logitech Public Source License"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r4"

DEPENDS += "squeezeplay"

SRC_URI="${SQUEEZEPLAY_SCM};module=squeezeplay_squeezeos"

S = "${WORKDIR}/squeezeplay_squeezeos"

inherit autotools

EXTRA_OEMAKE = "all lua-lint"


do_install() {
	autotools_do_install

	# move lua libraries to correct location
	rm ${D}${libdir}/* 
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 .libs/jiveWireless.so ${D}${libdir}/lua/5.1/jiveWireless.so
        install -m 0755 .libs/squeezeos_bsp.so ${D}${libdir}/lua/5.1/squeezeos_bsp.so
}


FILES_${PN} += "${datadir} ${libdir}/lua/5.1"
FILES_${PN}-dbg += "${libdir}/lua/5.1/.debug"
