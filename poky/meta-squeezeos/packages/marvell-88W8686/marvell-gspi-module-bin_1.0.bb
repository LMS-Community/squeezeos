DESCRIPTION = "Marvell 88W8686 gspi wlan drivers"
SECTION = "base"
LICENSE = "binary only"

PR = "r2"

PROVIDES = "marvell-gspi-module"

RDEPENDS = "wireless-tools"

SRC_URI=" \
	file://gspi.ko \
	file://gspi8686.bin \
	file://gspi8xxx.ko \
	file://helper_gspi.bin \
	"

inherit module-base

INHIBIT_PACKAGE_STRIP = 1

dirs = "/lib /lib/modules /lib/modules/${KERNEL_VERSION} /lib/firmware"

do_install() {
	for d in ${dirs}; do
		install -m 0755 -d ${D}$d
	done

	install -m 0755 ${WORKDIR}/gspi.ko ${D}/lib/modules/${KERNEL_VERSION}/gspi.ko
	install -m 0755 ${WORKDIR}/gspi8xxx.ko ${D}/lib/modules/${KERNEL_VERSION}/gspi8xxx.ko
	install -m 0755 ${WORKDIR}/gspi8686.bin ${D}/lib/firmware/gspi8686.bin
	install -m 0755 ${WORKDIR}/helper_gspi.bin ${D}/lib/firmware/helper_gspi.bin
}

PACKAGES = "marvell-gspi-module-dbg marvell-gspi-module"

FILES_marvell-gspi-module = "${base_libdir}/modules/${KERNEL_VERSION} ${base_libdir}/firmware"
FILES_marvell-gspi-module-dbg = "${base_libdir}/modules/${KERNEL_VERSION}/.debug"
