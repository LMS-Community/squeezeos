DESCRIPTION = "Sub::Name - (re)name a sub"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r2"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/X/XM/XMATH/Sub-Name-${PV}.tar.gz"

S = "${WORKDIR}/Sub-Name-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
