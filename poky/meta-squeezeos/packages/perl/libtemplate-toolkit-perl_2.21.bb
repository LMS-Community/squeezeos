DESCRIPTION = "Template - Front-end module to Template Toolkit"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r4"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/A/AB/ABW/Template-Toolkit-${PV}.tar.gz"

S = "${WORKDIR}/Template-Toolkit-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
