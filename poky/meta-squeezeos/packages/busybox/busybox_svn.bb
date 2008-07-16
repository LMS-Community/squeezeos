require busybox.inc
PR = "r9"

BV = "1_7_stable"

SRC_URI = "${SQUEEZEOS_SCM};module=busybox_${BV} \
           file://defconfig"

S = "${WORKDIR}/busybox_${BV}"

EXTRA_OEMAKE += "V=1 ARCH=${TARGET_ARCH} CROSS_COMPILE=${TARGET_PREFIX}"

do_configure () {
	install -m 0644 ${WORKDIR}/defconfig ${S}/.config
	cml1_do_configure
}
