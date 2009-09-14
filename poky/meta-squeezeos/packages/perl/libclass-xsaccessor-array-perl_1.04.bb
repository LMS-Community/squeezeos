DESCRIPTION = "Class::XSAccessor::Array - Generate fast XS accessors without runtime compilation"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r1"

DEPENDS = "libautoxs-header-perl-native"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/S/SM/SMUELLER/Class-XSAccessor-Array-${PV}.tar.gz"

S = "${WORKDIR}/Class-XSAccessor-Array-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
