DESCRIPTION = "HTML::Parser - HTML parser class"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r2"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/G/GA/GAAS/HTML-Parser-${PV}.tar.gz"

S = "${WORKDIR}/HTML-Parser-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
