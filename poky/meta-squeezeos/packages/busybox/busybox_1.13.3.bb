require busybox.inc
PR = "r4"

SRC_URI = "http://www.busybox.net/downloads/busybox-${PV}.tar.gz \
	   file://busybox_udhcpd_syslog.patch;patch=1 \
	   file://busybox_ifupdown_hostname.patch;patch=1 \
	   file://busybox_udhcpd_hostname_nak.patch;patch=1 \
           file://defconfig"

S = "${WORKDIR}/busybox-${PV}"

EXTRA_OEMAKE += "V=1 ARCH=${TARGET_ARCH} CROSS_COMPILE=${TARGET_PREFIX}"

do_configure () {
	install -m 0644 ${WORKDIR}/defconfig ${S}/.config
	cml1_do_configure
}
