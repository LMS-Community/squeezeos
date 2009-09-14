DESCRIPTION = "SqueezePlay - Private code"
LICENSE = "Confidential"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r11"

# don't use thumb for decoders
ARM_INSTRUCTION_SET = "arm"

DEPENDS += "libsdl lua axtls"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=squeezeplay_private"

S = "${WORKDIR}/squeezeplay_private"

inherit autotools

EXTRA_OECONF = "--disable-shared --enable-wma --enable-aac"
# --enable-profiling"

EXTRA_OECONF_fab4 = "--disable-shared --enable-wma --enable-aac --enable-chiral"

CFLAGS_prepend_fab4 = "-DIMX31"
CFLAGS_prepend_baby = "-DIMX25"

do_stage() {
	autotools_stage_all
	install -m 0644 src/syna_chiral_api.h ${STAGING_INCDIR}/syna_chiral_api.h

}

# Just build a shared library, don't install any packages
FILES_${PN} = ""
FILES_${PN}-dbg = ""
