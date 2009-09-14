DESCRIPTION = "Marvell wps app"
SECTION = "base"
LICENSE = "binary only"

PV = "3.11"
PR = "r1"

PROVIDES = "marvell-wps"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=src_wps"

S = "${WORKDIR}/src_wps"

do_compile() {
	# Compile encrypt_lib
#	oe_runmake encrypt_src
	cd ${S}/encrypt_src
#	make clean
	make build

	# Compile wpsapp
#	oe_runmake .
	cd ${S}/.
	make clean
	make build
}

do_install() {
	install -m 0755 -d ${D}${sbindir}

	# Install wpsapp
	install -m 0755 -d ${D}${sbindir}/wps
	install -m 0755 -d ${D}${sbindir}/wps/config

	install -m 0755 ${S}/wps/wpsapp ${D}${sbindir}/wps/wpsapp
	install -m 0755 ${S}/wps/config/wps_init.conf ${D}${sbindir}/wps/config/wps_init.conf

}

PACKAGES = "marvell-wps-dbg marvell-wps"

FILES_marvell-wps = "${sbindir}"
FILES_marvell-wps-dbg = "${sbindir}/wps/.debug"

