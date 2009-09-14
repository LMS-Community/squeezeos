DESCRIPTION = "Marvell 8686 wlan tools"
SECTION = "base"
LICENSE = "binary only"

PR = "r4"

PROVIDES = "marvell-wlan-tools"

SRC_URI=" \
	file://wlanconfig \
	file://wpa_cli \
	file://wpa_supplicant \
	"

dirs = "/usr /usr/sbin"

do_install() {
	for d in ${dirs}; do
		install -m 0755 -d ${D}$d
	done

	install -m 0755 ${WORKDIR}/wlanconfig ${D}${sbindir}/wlanconfig
	install -m 0755 ${WORKDIR}/wpa_cli ${D}${sbindir}/wpa_cli
	install -m 0755 ${WORKDIR}/wpa_supplicant ${D}${sbindir}/wpa_supplicant
}

PACKAGES = "marvell-wlan-tools-dbg marvell-wlan-tools"

FILES_marvell-wlan-tools = "${sbindir}"
FILES_marvell-wlan-tools-dbg = "${sbindir}/.debug"
