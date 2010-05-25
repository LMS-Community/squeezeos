DESCRIPTION = "Marvell 8686 wlan tools"
SECTION = "base"
LICENSE = "binary only"

PV = "1.0"
PR = "r5"

PROVIDES = "marvell-wlan-tools"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=src_gspi8686 \
	file://allow-special-char-in-ssid.patch;patch=1 \
	file://config-file-sync.patch;patch=1 \
"

S = "${WORKDIR}/src_gspi8686"

# With the csl2010q1 and high optimization it fails. 'arm' instruction set to be safe
ARM_INSTRUCTION_SET = "arm"
FULL_OPTIMIZATION = "-O1 -ggdb"

CFLAGS_prepend += "-I${S}/os/linux"
CFLAGS_prepend += "-I${S}/wlan"

do_compile() {
	# Compile wlanconfig
	oe_runmake app/wlanconfig

	# Compile wpa_supplicant. oe_runmake fails here, just use make
	cd ${S}/app/wpa_supplicant-0.5.7
	make
}

do_install() {
	install -m 0755 -d ${D}${sbindir}

	# Install wlanconfig
	install -m 0755 ${S}/app/wlanconfig/wlanconfig ${D}${sbindir}/wlanconfig

	# Install wpa_supplicant
	install -m 0755 ${S}/app/wpa_supplicant-0.5.7/wpa_cli ${D}${sbindir}/wpa_cli
	install -m 0755 ${S}/app/wpa_supplicant-0.5.7/wpa_supplicant ${D}${sbindir}/wpa_supplicant
}

PACKAGES = "marvell-wlan-tools-dbg marvell-wlan-tools"

FILES_marvell-wlan-tools = "${sbindir}"
FILES_marvell-wlan-tools-dbg = "${sbindir}/.debug"
