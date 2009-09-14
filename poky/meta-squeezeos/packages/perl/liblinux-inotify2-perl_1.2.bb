DESCRIPTION = "Linux::Inotify2 - scalable directory/file change notification"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r1"

SRC_URI = "http://backpan.perl.org/authors/id/M/ML/MLEHMANN/Linux-Inotify2-${PV}.tar.gz"

S = "${WORKDIR}/Linux-Inotify2-${PV}"

inherit cpan

FILES_${PN} = "${PERLLIBDIRS}/auto/Linux/Inotify2/* \
               ${PERLLIBDIRS}/Linux"
