DESCRIPTION = "LibTomMath is a free open source portable number theoretic multiple-precision integer library written entirely in C."
SECTION = "libs/network"
PRIORITY = "optional"
LICENSE = "PD"
PR = "r0"

ARM_INSTRUCTION_SET = "arm"

SRC_URI = "http://libtom.org/files/ltm-${PV}.tar.bz2"

inherit autotools

#FIXME add patch to make it shared

do_stage() {
	oe_libinstall -a libtommath ${STAGING_LIBDIR}/
	install -m 0644 ${S}/*.h ${STAGING_INCDIR}/
}

do_install() {
	:
}

