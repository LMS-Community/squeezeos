DESCRIPTION = "Class::XSAccessor - Generate fast XS accessors without runtime compilation"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r2"

DEPENDS = "libautoxs-header-perl-native"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/S/SM/SMUELLER/Class-XSAccessor-${PV}.tar.gz"

S = "${WORKDIR}/Class-XSAccessor-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
