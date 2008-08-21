DESCRIPTION = "Jive test software"

PV = "${DISTRO_VERSION}+svnr${SRCREV}"
PR = "r2"

SRC_URI="${SQUEEZEOS_SVN};module=jivetest;proto=http"

S = "${WORKDIR}/jivetest"

EXTRA_OEMAKE = "jivectl testir evtest"

do_install() {
	install -m 0755 -d ${D}${bindir}
	install -m 0755 ${S}/jivectl ${D}${bindir}/jivectl
	install -m 0755 ${S}/testir ${D}${bindir}/testir
	install -m 0755 ${S}/evtest ${D}${bindir}/evtest
}
