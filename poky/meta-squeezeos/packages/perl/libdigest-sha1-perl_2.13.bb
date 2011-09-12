DESCRIPTION = "Digest::SHA1 - Perl interface to the SHA-1 algorithm"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r17"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/G/GA/GAAS/Digest-SHA1-${PV}.tar.gz"

S = "${WORKDIR}/Digest-SHA1-${PV}"

inherit cpan

FILES_${PN} = "${PERLLIBDIRS}/auto/Digest/SHA1/* \
               ${PERLLIBDIRS}/Digest"
