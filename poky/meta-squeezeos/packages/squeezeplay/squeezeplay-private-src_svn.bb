DESCRIPTION = "SqueezePlay - Private code"
LICENSE = "Confidential"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
#PR = "r0"

DEPENDS += "squeezeplay axtls"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=squeezeplay_private"

S = "${WORKDIR}/squeezeplay_private"

inherit autotools

CFLAGS_prepend = '-I${STAGING_INCDIR}/squeezeplay'

EXTRA_OEMAKE = "all"


do_install() {
	autotools_do_install

	# move lua libraries to correct location
	rm ${D}${libdir}/* 
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 .libs/spprivate.so ${D}${libdir}/lua/5.1/spprivate.so
}


FILES_${PN} += "${datadir} ${libdir}/lua/5.1"
FILES_${PN}-dbg += "${libdir}/lua/5.1/.debug"
