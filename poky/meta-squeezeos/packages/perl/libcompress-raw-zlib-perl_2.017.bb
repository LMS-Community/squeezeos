DESCRIPTION = "Compress::Raw::Zlib - Low-Level Interface to zlib compression library"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r13"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/P/PM/PMQS/Compress-Raw-Zlib-${PV}.tar.gz"

S = "${WORKDIR}/Compress-Raw-Zlib-${PV}"

inherit cpan

FILES_${PN} = "${PERLLIBDIRS}/auto/Compress/Raw/Zlib/* \
               ${PERLLIBDIRS}/Compress"
