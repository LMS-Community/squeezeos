DESCRIPTION = "Template - Front-end module to Template Toolkit"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r2"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/A/AB/ABW/Template-Toolkit-${PV}.tar.gz"

S = "${WORKDIR}/Template-Toolkit-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
