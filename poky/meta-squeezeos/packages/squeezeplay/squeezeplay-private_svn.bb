DESCRIPTION = "SqueezePlay - Private code"
LICENSE = "Confidential"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
#PR = "r0"

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
