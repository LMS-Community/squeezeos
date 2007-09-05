/* vi: set sw=4 ts=4: */
/*
 * arpping.c
 *
 * Mostly stolen from: dhcpcd - DHCP client daemon
 * by Yoichi Hariguchi <yoichi@fore.com>
 */

#include <netinet/if_ether.h>
#include <net/if_arp.h>

#include "common.h"
#include "dhcpd.h"
#include "arpping.h"



/* Returns 1 if no reply received */

int arpping(uint32_t test_ip, uint32_t from_ip, uint8_t *from_mac, const char *interface)
{
	int timeout = 2;
	int s;                  /* socket */
	int rv = 1;             /* "no reply received" yet */
	struct sockaddr addr;   /* for interface name */
	struct arpMsg arp;
	fd_set fdset;
	struct timeval tm;
	unsigned prevTime;

	s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));
	if (s == -1) {
		bb_perror_msg(bb_msg_can_not_create_raw_socket);
		return -1;
	}

	if (setsockopt_broadcast(s) == -1) {
		bb_perror_msg("cannot setsocketopt on raw socket");
		goto ret;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memset(arp.h_dest, 0xff, 6);                    /* MAC DA */
	memcpy(arp.h_source, from_mac, 6);              /* MAC SA */
	arp.h_proto = htons(ETH_P_ARP);                 /* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);                /* hardware type */
	arp.ptype = htons(ETH_P_IP);                    /* protocol type (ARP message) */
	arp.hlen = 6;                                   /* hardware address length */
	arp.plen = 4;                                   /* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);           /* ARP op code */
	memcpy(arp.sHaddr, from_mac, 6);                /* source hardware address */
	memcpy(arp.sInaddr, &from_ip, sizeof(from_ip)); /* source IP address */
	/* tHaddr */                                    /* target hardware address */
	memcpy(arp.tInaddr, &test_ip, sizeof(test_ip)); /* target IP address */

	memset(&addr, 0, sizeof(addr));
	safe_strncpy(addr.sa_data, interface, sizeof(addr.sa_data));
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		goto ret;

	/* wait for arp reply, and check it */
	do {
		int r;
		prevTime = monotonic_sec();
		FD_ZERO(&fdset);
		FD_SET(s, &fdset);
		tm.tv_sec = timeout;
		tm.tv_usec = 0;
		r = select(s + 1, &fdset, NULL, NULL, &tm);
		if (r < 0) {
			bb_perror_msg("error on ARPING request");
			if (errno != EINTR)
				break;
		} else if (r) {
			if (recv(s, &arp, sizeof(arp), 0) < 0)
				break;
			if (arp.operation == htons(ARPOP_REPLY)
			 && memcmp(arp.tHaddr, from_mac, 6) == 0
			 && *((uint32_t *) arp.sInaddr) == test_ip
			) {
				rv = 0;
				break;
			}
		}
		timeout -= monotonic_sec() - prevTime;
	} while (timeout > 0);

 ret:
	close(s);
	DEBUG("%srp reply received for this address", rv ? "No a" : "A");
	return rv;
}
