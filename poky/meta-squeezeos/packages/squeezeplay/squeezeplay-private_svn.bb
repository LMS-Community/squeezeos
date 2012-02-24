DESCRIPTION = "SqueezePlay - Private code"
LICENSE = "Confidential"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r17"

# don't use thumb for decoders
ARM_INSTRUCTION_SET = "arm"

# Optimize more
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

# For profiling:
#FULL_OPTIMIZATION = "-fexpensive-optimizations -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

DEPENDS += "libsdl lua axtls openssl gmp beecrypt libspotify"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=squeezeplay_private"

S = "${WORKDIR}/squeezeplay_private"

inherit autotools

EXTRA_OECONF = "--disable-shared --enable-wma --enable-aac --enable-arm"
# --enable-profiling"

EXTRA_OECONF_fab4 = "--disable-shared --enable-wma --enable-aac --enable-arm --enable-chiral" 

# Enable extra AAC experimental optimizations
#CFLAGS_prepend_fab4 = "-DIMX31"
#CFLAGS_prepend_baby = "-DIMX25"

# We need to override the optimization for the build of the WMA fft.c module
# because we get bad code generation with any optimization enabled.
# Override CFLAGS here so that we can pass the desired flags via the environment (TARGET_CFLAGS)
# at compilation time instead of autoconf building them into the Makefile at configure time;
# Makefile.am now uses TARGET_CFLAGS and TARGET_CFLAGS_WMAFFT
# -pass-exit-codes is just a dummy value so that configure does not generate defaults
CFLAGS = -pass-exit-codes
export TARGET_CFLAGS_WMAFFT = ${TARGET_CFLAGS} -O0

do_stage() {
	autotools_stage_all
	install -m 0644 src/syna_chiral_api.h ${STAGING_INCDIR}/syna_chiral_api.h

}

# Just build a shared library, don't install any packages
FILES_${PN} = ""
FILES_${PN}-dbg = ""
