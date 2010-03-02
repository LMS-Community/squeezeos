DESCRIPTION = "YAML::Syck - Fast, lightweight YAML loader and dumper"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r3"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/A/AU/AUDREYT/YAML-Syck-${PV}.tar.gz"

S = "${WORKDIR}/YAML-Syck-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
