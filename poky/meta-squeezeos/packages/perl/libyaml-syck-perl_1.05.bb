DESCRIPTION = "YAML::Syck - Fast, lightweight YAML loader and dumper"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r1"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/A/AU/AUDREYT/YAML-Syck-${PV}.tar.gz"

S = "${WORKDIR}/YAML-Syck-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
