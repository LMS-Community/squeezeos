SECTION = "libs"
PRIORITY = "optional"
DESCRIPTION = "tremor is a low memory fixed point implementation of the vorbis codec."
LICENSE = "BSD"
##SRCDATE = "${PV}"
PR = "r2"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"

SRC_URI = "svn://svn.slimdevices.com/repos/jive/7.4/trunk/squeezeplay/src;module=Tremor;proto=http"

S = "${WORKDIR}/Tremor"

inherit autotools

EXTRA_OECONF=" --enable-shared --disable-rpath  "

do_stage() {
	autotools_stage_all
}

ARM_INSTRUCTION_SET = "arm"

