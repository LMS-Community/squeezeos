DESCRIPTION = "Encode::Detect - An Encode::Encoding subclass that detects the encoding of data"
SECTION = "libs"
LICENSE = "Artistic|GPL"
#PR = "r0"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/J/JG/JGMYERS/Encode-Detect-${PV}.tar.gz"

S = "${WORKDIR}/Encode-Detect-${PV}"

inherit cpan

FILES_${PN} = "${PERLLIBDIRS}/auto/Encode/Detect/* \
               ${PERLLIBDIRS}/Encode"
