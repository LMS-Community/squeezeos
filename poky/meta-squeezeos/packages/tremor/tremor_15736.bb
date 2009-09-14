SECTION = "libs"
PRIORITY = "optional"
DESCRIPTION = "tremor is a low memory fixed point implementation of the vorbis codec."
LICENSE = "BSD"
SRCDATE = "${PV}"
PR = "r1"

SRC_URI = "svn://svn.xiph.org/branches/lowmem-branch;module=Tremor;rev=${PV};proto=http"

S = "${WORKDIR}/Tremor"

inherit autotools

EXTRA_OECONF=" --enable-shared --disable-rpath  "

do_stage() {
	autotools_stage_all
}

ARM_INSTRUCTION_SET = "arm"

