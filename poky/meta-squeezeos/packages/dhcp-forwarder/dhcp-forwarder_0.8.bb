SECTION = "console/network"
DESCRIPTION = "This program is used to forward DHCP and BOOTP messages between two \
 networks with different broadcast domains. \
 It works better with ppp - and especially with ipsec over ppp - than \
 dhcp-relay from ISC and has a smaller foot print."
HOMEPAGE = "http://www.nongnu.org/dhcp-fwd/"
LICENSE = "GPLv3"

PR = "r0"

SRC_URI = "http://savannah.nongnu.org/download/dhcp-fwd/dhcp-forwarder-${PV}.tar.bz2 \
	file://bridge.patch;patch=0;pnum=0 \
	file://bridge \
	file://bridge_helper.sh \
	file://dhcp-fwd.cfg"

inherit autotools

EXTRA_OECONF="--disable-dietlibc"

do_install() {
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/bridge ${D}${sysconfdir}/init.d/bridge
	install -m 0644 ${WORKDIR}/dhcp-fwd.cfg ${D}${sysconfdir}/dhcp-fwd.cfg

	install -d ${D}${sbindir}/
	install -m 0755 ${S}/dhcp-fwd ${D}${sbindir}/dhcp-fwd
	install -m 0755 ${WORKDIR}/bridge_helper.sh ${D}${sbindir}/bridge_helper.sh
}

PACKAGES = "dhcp-forwarder-dbg dhcp-forwarder"

