DESCRIPTION = "Image::Scale - Fast, high-quality fixed-point image resizing"
SECTION = "libs"
LICENSE = "GPL"
PR = "r4"

DEPENDS = "jpeg libpng giflib"

ARM_INSTRUCTION_SET = "arm"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/A/AG/AGRUNDMA/Image-Scale-${PV}.tar.gz"

S = "${WORKDIR}/Image-Scale-${PV}"

inherit cpan

export INCLUDE = ${STAGING_LIBDIR}/../include

FILES_${PN} = "${PERLLIBDIRS}"
