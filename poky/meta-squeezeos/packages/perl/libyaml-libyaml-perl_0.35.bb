DESCRIPTION = "YAML::LibYAML"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r5"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/I/IN/INGY/YAML-LibYAML-${PV}.tar.gz"

S = "${WORKDIR}/YAML-LibYAML-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
