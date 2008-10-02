DESCRIPTION = "SqueezePlay - Jive specific code"
LICENSE = "Logitech Public Source License"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r6"

DEPENDS += "squeezeplay"

SRC_URI="${SQUEEZEPLAY_SCM};module=squeezeplay_jive"

S = "${WORKDIR}/squeezeplay_jive"

inherit autotools

CFLAGS_prepend = '-DSQUEEZEPLAY_RELEASE=\\"${DISTRO_VERSION}\\" -DSQUEEZEPLAY_REVISION=\\"${SQUEEZEOS_REVISION}\\"'

EXTRA_OEMAKE = "all lua-lint"

export PREFIX=%{D}

do_install() {
	autotools_do_install

	# move lua libraries to correct location
	rm ${D}${libdir}/* 
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 .libs/jiveBSP.so ${D}${libdir}/lua/5.1/jiveBSP.so
	install -m 0755 .libs/jiveWireless.so ${D}${libdir}/lua/5.1/jiveWireless.so
	install -m 0755 .libs/jiveWatchdog.so ${D}${libdir}/lua/5.1/jiveWatchdog.so
}


FILES_${PN} += "${datadir} ${libdir}/lua/5.1"
FILES_${PN}-dbg += "${libdir}/lua/5.1/.debug"
