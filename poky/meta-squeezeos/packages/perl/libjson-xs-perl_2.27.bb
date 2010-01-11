DESCRIPTION = "JSON::XS - JSON serialising/deserialising, done correctly and fast"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r3"

SRC_URI = "http://backpan.perl.org/authors/id/M/ML/MLEHMANN/JSON-XS-${PV}.tar.gz"

S = "${WORKDIR}/JSON-XS-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
