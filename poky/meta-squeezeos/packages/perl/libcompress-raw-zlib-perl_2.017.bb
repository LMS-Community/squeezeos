DESCRIPTION = "Compress::Raw::Zlib - Low-Level Interface to zlib compression library"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r10"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/P/PM/PMQS/Compress-Raw-Zlib-${PV}.tar.gz"

S = "${WORKDIR}/Compress-Raw-Zlib-${PV}"

inherit cpan

FILES_${PN} = "${PERLLIBDIRS}/auto/Compress/Raw/Zlib/* \
               ${PERLLIBDIRS}/Compress"
