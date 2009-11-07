DESCRIPTION = "Class::XSAccessor - Generate fast XS accessors without runtime compilation"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r3"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/S/SM/SMUELLER/Class-XSAccessor-${PV}_04.tar.gz"

S = "${WORKDIR}/Class-XSAccessor-${PV}_04"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
