DESCRIPTION = "Atheros AR63 sdio wlan driver"
SECTION = "base"
LICENSE = "binary only"

PR = "r4"

PROVIDES = "atheros-ar63-module"

DEPENDS = "virtual/kernel"
#RDEPENDS = "wireless-tools"

SRC_URI=" \
	 file://ar6000.ko \
	 file://bmiloader \
	 file://wmiconfig \
	 file://eeprom \
	 file://athwlan.bin \
	 file://data.patch.hw3_0.bin \
	 file://calData_WB44_030_D0400_spur_enabled_040312_2G_Only.bin \
	 file://otp.bin \
	 file://loadAR6000l.sh \
	 file://wlan \
"

inherit module-base

INHIBIT_PACKAGE_STRIP = 1

do_install() {
	INSTALL_DIR=${D}/${base_libdir}/atheros3
	install -m 0755 -d ${INSTALL_DIR}

	# kernel module
	install -m 0644 ${WORKDIR}/ar6000.ko ${INSTALL_DIR}/ar6000.ko

	# tools
	install -m 0755 ${WORKDIR}/bmiloader ${INSTALL_DIR}/bmiloader
	install -m 0755 ${WORKDIR}/wmiconfig ${INSTALL_DIR}/wmiconfig
	install -m 0755 ${WORKDIR}/eeprom ${INSTALL_DIR}/eeprom

	# firmware
	install -m 0644 ${WORKDIR}/athwlan.bin ${INSTALL_DIR}/athwlan.bin
	install -m 0644 ${WORKDIR}/data.patch.hw3_0.bin ${INSTALL_DIR}/data.patch.hw3_0.bin
	install -m 0644 ${WORKDIR}/otp.bin ${INSTALL_DIR}/otp.bin

	# scripts
	install -m 0644 ${WORKDIR}/calData_WB44_030_D0400_spur_enabled_040312_2G_Only.bin ${INSTALL_DIR}/calData_WB44_030_D0400_spur_enabled_040312_2G_Only.bin
	install -m 0755 ${WORKDIR}/loadAR6000l.sh ${INSTALL_DIR}/loadAR6000l.sh

	install -m 0755 -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/wlan ${D}${sysconfdir}/init.d/wlan
}


PACKAGES = "atheros-ar63-module-dbg atheros-ar63-module"

FILES_atheros-ar63-module = "${base_libdir}/modules/${KERNEL_VERSION} ${base_libdir}/atheros3 ${sysconfdir}"
FILES_atheros-ar63-module-dbg = "${base_libdir}/modules/${KERNEL_VERSION}/.debug ${base_libdir}/atheros3/.debug"
