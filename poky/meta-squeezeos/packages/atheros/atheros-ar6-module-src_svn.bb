DESCRIPTION = "Atheros AR6 sdio wlan driver"
SECTION = "base"
LICENSE = "binary only"

PV = "1.0"
PR = "r21"

PROVIDES = "atheros-ar6-module"

DEPENDS = "virtual/kernel"
#RDEPENDS = "wireless-tools"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=atheros/AR6kSDK.build_sw.62 \
	 file://AR6kSDK.build_sw.62.baby.patch;patch=1 \
	 file://BMI_read_mem_failure.patch;patch=1 \
	 file://atheros_scan_hidden_ssid.patch;patch=1 \
	 file://change-ar6k-semaphore-type.patch;patch=1 \
	 file://atheros_disable_tcmd.patch;patch=1 \
	 file://atheros_shorter_scan_list_cache_timeout.patch;patch=1 \
	 file://atheros_send_disconnect_event.patch;patch=1 \
	 file://wlan \
	 file://calData_ar6102_15dBm.bin \
	 file://loadAR6000l.sh \
"

S = "${WORKDIR}/AR6kSDK.build_sw.62"

inherit module-base

INHIBIT_PACKAGE_STRIP = 1

do_compile() {
	cd ${S}/host
	oe_runmake LDFLAGS=""
}

do_install() {
	INSTALL_DIR=${D}/${base_libdir}/atheros
	install -m 0755 -d ${INSTALL_DIR}

	# kernel module
	install -m 0644 ${S}/host/.output/NATIVEMMC-SDIO/image/ar6000.ko ${INSTALL_DIR}/ar6000.ko

	# tools
	install -m 0755 ${S}/host/.output/NATIVEMMC-SDIO/image/bmiloader ${INSTALL_DIR}/bmiloader
	install -m 0755 ${S}/host/.output/NATIVEMMC-SDIO/image/wmiconfig ${INSTALL_DIR}/wmiconfig
	install -m 0755 ${S}/host/.output/NATIVEMMC-SDIO/image/eeprom.AR6002 ${INSTALL_DIR}/eeprom.AR6002

	# firmware
	install -m 0644 ${S}/target/AR6002/hw2.0/bin/athwlan.bin ${INSTALL_DIR}/athwlan.bin
	install -m 0644 ${S}/target/AR6002/hw2.0/bin/data.patch.hw2_0.bin ${INSTALL_DIR}/data.patch.hw2_0.bin

	# scripts
	install -m 0644 ${WORKDIR}/calData_ar6102_15dBm.bin ${INSTALL_DIR}/calData_ar6102_15dBm.bin
	install -m 0755 ${WORKDIR}/loadAR6000l.sh ${INSTALL_DIR}/loadAR6000l.sh

	install -m 0755 -d ${D}${sysconfdir}/init.d
## The wlan script file is copied in the atheros-ar63-module-bin_1.0.bb recipe.
## It will select the proper driver according to board revision.
#	install -m 0755 ${WORKDIR}/wlan ${D}${sysconfdir}/init.d/wlan
}


PACKAGES = "atheros-ar6-module-dbg atheros-ar6-module"

FILES_atheros-ar6-module = "${base_libdir}/modules/${KERNEL_VERSION} ${base_libdir}/atheros ${sysconfdir}"
FILES_atheros-ar6-module-dbg = "${base_libdir}/modules/${KERNEL_VERSION}/.debug ${base_libdir}/atheros/.debug"
