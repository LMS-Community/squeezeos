DESCRIPTION = "Marvell 88W8686 sdio wlan driver"
SECTION = "base"
LICENSE = "GPL"

PV = "1.0"
PR = "r4"

PROVIDES = "marvell-sdio-module"

DEPENDS = "virtual/kernel"
RDEPENDS = "wireless-tools marvell-wlan-tools"

SRC_URI = "${SQUEEZEOS_SVN};module=marvell_SD8686_v9;proto=https \
	file://wlan-sdio \
	"

S = "${WORKDIR}/marvell_SD8686_v9"

inherit module-base

INHIBIT_PACKAGE_STRIP = 1

CFLAGS_prepend += "-I${S}/os/linux"
CFLAGS_prepend += "-I${S}/wlan"

do_compile() {
	# Compile kernel modules
        # ${KERNEL_LD} doesn't understand the LDFLAGS, so suppress them
        oe_runmake CC="${KERNEL_CC}" LD="${KERNEL_LD}" LDFLAGS="" KERNELDIR=${KERNEL_SOURCE} MODDIR=/${base_libdir}/modules/${KERNEL_VERSION}
}

do_install() {
	# Install kernel modules
	install -m 0755 -d ${D}/${base_libdir}/modules/${KERNEL_VERSION}
	install -m 0644 ${S}/sd8686.ko ${D}/${base_libdir}/modules/${KERNEL_VERSION}

	# Install wlan firmware
	install -m 0755 -d ${D}/${base_libdir}/firmware
	install -m 0644 ${S}/FwImage/sd8686.bin ${D}/${base_libdir}/firmware/sd8686.bin
	install -m 0644 ${S}/FwImage/helper_sd.bin ${D}/${base_libdir}/firmware/helper_sd.bin

	# Install init script
	install -m 0755 -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/wlan-sdio ${D}${sysconfdir}/init.d/wlan
}

PACKAGES = "marvell-sdio-module-dbg marvell-sdio-module"

FILES_marvell-sdio-module = "${base_libdir}/modules/${KERNEL_VERSION} ${base_libdir}/firmware ${sysconfdir}"
FILES_marvell-sdio-module-dbg = "${base_libdir}/modules/${KERNEL_VERSION}/.debug"
