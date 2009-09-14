DESCRIPTION="A simple command to run on Unix systems that will check the time \
and optionally drift compared with a known, local and reliable NTP \
time server." 
LICENSE = "individual"
PR = "r1"

SRC_URI = "ftp://ftp.openpkg.org/sources/DST/msntp/msntp-${PV}.tar.gz"

export LIBS = "-lm"

PACKAGES = "msntp"

FILES_msntp = "${sbindir}/msntp"

do_install () {
	# Install msntp
	install -m 0755 -d ${D}${sbindir}
	install -m 0755 ${S}/msntp ${D}${sbindir}/msntp
}