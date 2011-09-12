DESCRIPTION = "Sub::Name - (re)name a sub"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r6"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/F/FL/FLORA/Sub-Name-${PV}.tar.gz"

S = "${WORKDIR}/Sub-Name-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
