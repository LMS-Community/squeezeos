DESCRIPTION = "TomsFastMath is a fast public domain, open source, large integer arithmetic library written in portable ISO C. It is a port of LibTomMath with optional support for inline assembler multipliers."
SECTION = "libs/network"
PRIORITY = "optional"
LICENSE = "PD"
PR = "r2"

ARM_INSTRUCTION_SET = "arm"

SRC_URI = "http://libtom.org/files/tfm-${PV}.tar.bz2"

S = "${WORKDIR}/tomsfastmath-${PV}"

inherit autotools

do_stage() {
	oe_libinstall -a libtfm ${STAGING_LIBDIR}/
	install -m 0644 ${S}/src/headers/*.h ${STAGING_INCDIR}/
}

do_install() {
	:
}

