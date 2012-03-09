DESCRIPTION = "LibTomCrypt is a fairly comprehensive, modular \
and portable cryptographic toolkit that provides developers \
with a vast array of well known published block ciphers, \
one-way hash functions, chaining modes, pseudo-random number \
generators, public key cryptography and a plethora of other \
routines."
SECTION = "libs/network"
PRIORITY = "optional"
DEPENDS = "libtommath"
LICENSE = "PD"
PR = "r4"

ARM_INSTRUCTION_SET = "arm"

SRC_URI = "git://github.com/libtom/libtomcrypt.git;protocol=git;tag=5c9fa403ff5f0d465afa04f10e2e33b3fb3cb69d"

S = "${WORKDIR}/libtomcrypt/"

inherit autotools

CFLAGS_prepend = "-DLTM_DESC -DUSE_LTM"

EXTRA_OEMAKE = "library"

#FIXME add patch to make it shared

do_stage() {
	oe_libinstall -a libtomcrypt ${STAGING_LIBDIR}/
	install -m 0644 ${S}/src/headers/*.h ${STAGING_INCDIR}/
}

do_install() {
	:
}

