DESCRIPTION = "GD - Interface to Gd Graphics Library"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r1"

DEPENDS = "gd"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/L/LD/LDS/GD-${PV}.tar.gz"

S = "${WORKDIR}/GD-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
