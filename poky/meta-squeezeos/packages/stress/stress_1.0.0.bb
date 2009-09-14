DESCRIPTION = "stress"
SECTION = "bin"
LICENSE = "GPL"

#PR = "r0"

SRC_URI="http://weather.ou.edu/~apw/projects/stress/stress-${PV}.tar.gz"

S = "${WORKDIR}/stress-${PV}"

inherit autotools

PACKAGES = "stress"
FILES_${PN} = "${bindir}/*"
