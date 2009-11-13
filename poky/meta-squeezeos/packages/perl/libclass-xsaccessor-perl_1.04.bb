DESCRIPTION = "Class::XSAccessor - Generate fast XS accessors without runtime compilation"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r5"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/S/SM/SMUELLER/Class-XSAccessor-${PV}_05.tar.gz"

S = "${WORKDIR}/Class-XSAccessor-${PV}_05"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
