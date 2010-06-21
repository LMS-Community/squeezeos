DESCRIPTION = "MP3::Cut::Gapless - Split an MP3 file without gaps (based on pcutmp3)"
SECTION = "libs"
LICENSE = "GPL"
PR = "r1"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/A/AG/AGRUNDMA/MP3-Cut-Gapless-${PV}.tar.gz"

S = "${WORKDIR}/MP3-Cut-Gapless-${PV}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
