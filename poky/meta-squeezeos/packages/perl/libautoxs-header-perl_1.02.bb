DESCRIPTION = "AutoXS::Header - Container for the AutoXS header files"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r1"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/S/SM/SMUELLER/AutoXS-Header-${PV}.tar.gz"

S = "${WORKDIR}/AutoXS-Header-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
