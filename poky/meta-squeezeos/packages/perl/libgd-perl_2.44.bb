DESCRIPTION = "GD - Interface to Gd Graphics Library"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r5"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

DEPENDS = "gd"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/L/LD/LDS/GD-${PV}.tar.gz \
           file://libgd-perl-scaling.patch;patch=1"

S = "${WORKDIR}/GD-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
