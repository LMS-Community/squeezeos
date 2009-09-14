DESCRIPTION = "Marvell wps app"
SECTION = "base"
LICENSE = "binary only"

PR = "r1"

PROVIDES = "marvell-wps"

SRC_URI=" \
	file://wpsapp \
	file://wps_init.conf \
	"

do_install() {
	install -m 0755 -d ${D}${sbindir}

	# Install wpsapp
	install -m 0755 -d ${D}${sbindir}/wps
	install -m 0755 -d ${D}${sbindir}/wps/config

	install -m 0755 ${WORKDIR}/wpsapp ${D}${sbindir}/wps/wpsapp
	install -m 0755 ${WORKDIR}/wps_init.conf ${D}${sbindir}/wps/config/wps_init.conf
}

PACKAGES = "marvell-wps-dbg marvell-wps"

FILES_marvell-wps = "${sbindir}"
FILES_marvell-wps-dbg = "${sbindir}/wps/.debug"
