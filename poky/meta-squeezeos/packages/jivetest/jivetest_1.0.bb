DESCRIPTION = "Jive test software"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r3"

SRC_URI="${SQUEEZEOS_SVN};module=jivetest;proto=https"

S = "${WORKDIR}/jivetest"

EXTRA_OEMAKE = "jivectl testir evtest memtool"

do_install() {
	install -m 0755 -d ${D}${bindir}
	install -m 0755 ${S}/jivectl ${D}${bindir}/jivectl
	install -m 0755 ${S}/testir ${D}${bindir}/testir
	install -m 0755 ${S}/evtest ${D}${bindir}/evtest
	install -m 0755 ${S}/memtool ${D}${bindir}/memtool
}

FILES_${PN} = "${bindir}/evtest \
	       ${bindir}/memtool"

FILES_${PN}_jive = "${bindir}/evtest \
		    ${bindir}/memtool \
		    ${bindir}/jivectl \
		    ${bindir}/testir"
