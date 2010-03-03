DESCRIPTION = "JSON::XS - JSON serialising/deserialising, done correctly and fast"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r5"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://backpan.perl.org/authors/id/M/ML/MLEHMANN/JSON-XS-${PV}.tar.gz"

S = "${WORKDIR}/JSON-XS-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
