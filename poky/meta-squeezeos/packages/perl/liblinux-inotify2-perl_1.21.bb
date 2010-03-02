DESCRIPTION = "Linux::Inotify2 - scalable directory/file change notification"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r4"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://backpan.perl.org/authors/id/M/ML/MLEHMANN/Linux-Inotify2-${PV}.tar.gz"

S = "${WORKDIR}/Linux-Inotify2-${PV}"

inherit cpan

FILES_${PN} = "${PERLLIBDIRS}/auto/Linux/Inotify2/* \
               ${PERLLIBDIRS}/Linux"
