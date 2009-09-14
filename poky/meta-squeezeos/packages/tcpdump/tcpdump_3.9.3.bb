DESCRIPTION = "Network Packet Capture"
HOMEPAGE = "http://www.tcpdump.org/"
LICENSE = "BSD"
SECTION = "libs/network"
PR = "r1"

SRC_URI = "http://www.tcpdump.org/release/tcpdump-${PV}.tar.gz"

DEPENDS = "libpcap"

inherit autotools

CPPFLAGS_prepend = "-I${S} "
CFLAGS_prepend = "-I${S} "
CXXFLAGS_prepend = "-I${S} "

ARM_INSTRUCTION_SET = "arm"

# I must be doing something wrong as only --disable-smb actually reduces the footprint of the exe by about 70k
#EXTRA_OECONF = " \
# --disable-802_11 --disable-cdp --disable-ether --disable-ipcomp --disable-lwres --disable-pim --disable-sl --disable-timed \
# --disable-ah --disable-chdlc --disable-fddi --disable-ipfc --disable-mobile --disable-ppp --disable-sll --disable-token \
# --disable-aodv --disable-cip --disable-frag6 --disable-ipx --disable-mobility --disable-pppoe --disable-slow --disable-udp \
# --disable-ap1394 --disable-cnfp --disable-fr --disable-isakmp --disable-mpls --disable-pptp --disable-smb --disable-vjc \
# --disable-arcnet --disable-decnet --disable-gre --disable-isoclns --disable-msdp --disable-radius --disable-snmp --disable-vrrp \
# --disable-arp --disable-dhcp6 --disable-hsrp --disable-juniper --disable-netbios --disable-raw --disable-stp --disable-wb \
# --disable-ascii --disable-domain --disable-icmp6 --disable-krb --disable-nfs --disable-rip --disable-sunatm --disable-zephyr \
# --disable-atalk --disable-dvmrp --disable-icmp --disable-l2tp --disable-ntp --disable-ripng --disable-sunrpc \
# --disable-atm --disable-eap --disable-igmp --disable-lane --disable-null --disable-rsvp --disable-symantec \
# --disable-beep --disable-egp --disable-igrp --disable-ldp --disable-ospf6 --disable-rt6 --disable-syslog \
# --disable-bfd --disable-eigrp --disable-ip6 --disable-llc --disable-ospf --disable-rx --disable-tcp \
# --disable-bgp --disable-enc --disable-ip6opts --disable-lmp --disable-pflog --disable-sctp --disable-telnet \
# --disable-bootp --disable-esp --disable-ip --disable-lspping --disable-pgm --disable-sip --disable-tftp"

EXTRA_OECONF = "--disable-smb"

do_configure_prepend () {
	if [ ! -e acinclude.m4 ]; then
		cat aclocal.m4 > acinclude.m4
	fi
}

do_compile () {
	make clean
	oe_runmake all
}

do_install () {
	install -d ${D}${sbindir}
	install -m 0755 ${S}/tcpdump ${D}${sbindir}/tcpdump
}

