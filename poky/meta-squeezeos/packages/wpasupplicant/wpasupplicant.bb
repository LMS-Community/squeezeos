DESCRIPTION = "wpasupplicant"
SECTION = "base"
LICENSE = "GNU GPL"

PV = "0.6.9"
PR = "r5"

PROVIDES = "wpasupplicant"

SRC_URI = "http://hostap.epitest.fi/releases/wpa_supplicant-${PV}.tar.gz \
	   file://wps.patch;patch=1;pnum=0 \
	   file://allow-special-char-in-ssid.patch;patch=1;pnum=0 \
	   file://config-file-sync.patch;patch=1;pnum=0 \
	   file://defconfig"

S = "${WORKDIR}/wpa_supplicant-${PV}/wpa_supplicant"

inherit autotools

# With the csl2010q1 and high optimization it fails. 'arm' instruction set to be safe
ARM_INSTRUCTION_SET = "arm"
FULL_OPTIMIZATION = "-O1 -ggdb"

do_configure () {
	install -m 0644 ${WORKDIR}/defconfig ${S}/.config
}

do_compile() {
##	make clean
	oe_runmake all
}

do_install() {
	install -m 0755 -d ${D}${sbindir}

	# Install wpa_supplicant, wpa_cli
	install -m 0755 ${S}/wpa_supplicant ${D}${sbindir}/wpa_supplicant
	install -m 0755 ${S}/wpa_cli ${D}${sbindir}/wpa_cli
}

PACKAGES = "wpasupplicant-dbg wpasupplicant"

FILES_wpasupplicant = "${sbindir}"
FILES_wpasupplicant-dbg = "${sbindir}.debug"

