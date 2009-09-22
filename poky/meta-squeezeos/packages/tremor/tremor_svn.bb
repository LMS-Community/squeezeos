SECTION = "libs"
PRIORITY = "optional"
DESCRIPTION = "tremor is a low memory fixed point implementation of the vorbis codec."
LICENSE = "BSD"
##SRCDATE = "${PV}"
PR = "r3"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"

SRC_URI = "${SQUEEZEPLAY_SCM};module=Tremor"

S = "${WORKDIR}/Tremor"

inherit autotools

EXTRA_OECONF=" --enable-shared --disable-rpath  "

CFLAGS_prepend = "-DSQUEEZEPLAY "

do_stage() {
	autotools_stage_all
}

ARM_INSTRUCTION_SET = "arm"

