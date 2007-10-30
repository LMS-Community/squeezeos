/* zeroconf.c
 *
 * Zeroconf handling
 * Based on draft-ietf-zeroconf-ipv4-linklocal.txt (July 2004)
 *
 * Elvis Pfützenreuter <elvis@indt.org> May 2005
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* TODO (future): Excess traffic in DHCP socket (while DHCP lease has not been granted)
 * TODO: Tests with autoimmune response (two network interfaces in the same net, with/wo 2 udhcpc)
 * TODO (future): ARP "is-at" in broadcast mode if using link-local address - move to kernel ?
 */

#include <sys/time.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

#include <net/if_arp.h>

#include "common.h"
#include "arpping.h"
#include "zeroconf.h"


/* Prototypes */
static void choose_addr(void);
static void disarm_timeout(void);
static int is_timeout_disarmed(void);
static int arp_conflict(void);
static void arp_probe(void);
static void arp_announce(struct in_addr *target);
static void initial_time(void);
static void probe(void);
static void active_announce(void);
static void active_rest(void);
static int timeout_passed(void);
static int timeout_exhausted(void);
static void _disable(void);
static void disable(void);
static void disable_permanently(void);
static void rate_limit(void);


/* static variables */

int state;
int conflicted;
int conflict_count;

int zeroconf_fd;
struct timeval timeout;
int timeout_count;
char* interface;
int ifindex;

struct in_addr linklocal_addr;
uint8_t mac_addr[6];
uint64_t mac_addr_as_int;


/* Inits zeroconf structures and state machine */

void zeroconf_init(int enable, uint8_t arp[], const char* pinterface, int pifindex)
{
	int i;

	if (enable) {
		state = ZEROCONF_SM_DISABLED;
	} else {
		state = ZEROCONF_SM_PERMANENTLY_DISABLED;
	}

	disarm_timeout();
	linklocal_addr.s_addr = 0;
	conflicted = 0;
	zeroconf_fd = -1;
	mac_addr_as_int = 0;
	
	conflict_count = 0;
	
	for(i = 0; i < 6; ++i) {
		mac_addr[i] = arp[i];
		mac_addr_as_int <<= 8;
		mac_addr_as_int += arp[i];
	}

	interface = strdup(pinterface);
	ifindex = pifindex;
}

/* generates address in 169.254.0.0/16 range, excluding 169.254.0.0/24 and
 * 169.254.255.0/24 edge ranges */

static void choose_addr(void)
{
	if (conflicted) {

		/* conflicts took place sometime, our MAC-derived address
		 * is not good enough, try random() for a change */

		linklocal_addr.s_addr = (169 << 24) + 
				(254 << 16) +
				256 + (random() % (65536 - 256 - 256));

		DEBUG("zeroconf: previous address conflicted, trying random this time");

	} else {
		/* calculate IP address based on MAC so it will be the same
		 * every time the machine is turned on */
		
		linklocal_addr.s_addr = (169 << 24) + 
				(254 << 16) +
				256 + ((mac_addr_as_int + 13371361) % (65536 - 256 - 256));
	}

	linklocal_addr.s_addr = htonl(linklocal_addr.s_addr);
	
	DEBUG("zeroconf: address selected is %s", inet_ntoa(linklocal_addr));
}

/* Helper function to set TIMEVAL structure to smallest timeout
 * between DHCP and zeroconf (NOTE: other_timeout is UNIX time seconds since 1970) */

void zeroconf_set_smallest_timeout(struct timeval* tv, unsigned int other_timeout)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	if (is_timeout_disarmed()) {
		tv->tv_sec = other_timeout;
		tv->tv_usec = 0;
	} else if (other_timeout <= timeout.tv_sec) {
		tv->tv_sec = other_timeout;
		tv->tv_usec = 0;
	} else {
		*tv = timeout;
	}

	/* we want differential timeout for select(), so we subtract time */
	tv->tv_sec -= now.tv_sec;
	tv->tv_usec -= now.tv_usec;

	while (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec -= 1;
	}

	if (tv->tv_sec < 0) {
		/* glup */
		tv->tv_sec = tv->tv_usec = 0;
	}

// DEBUG("zeroconf_set_smallest_timeout: o=%d, z=%d:%d, tv set to %d:%d, now is %d:%d", 
//			other_timeout, timeout.tv_sec, timeout.tv_usec,
// 			tv->tv_sec, tv->tv_usec, now.tv_sec, now.tv_usec);
}

/* Sets timeout to a moment in absolute time */

static void set_timeout(long int usecs, int times)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	timeout.tv_sec = now.tv_sec + (usecs / 1000000);
	timeout.tv_usec = now.tv_usec + (usecs % 1000000);

	if (timeout.tv_usec >= 1000000) {
		timeout.tv_usec -= 1000000;
		timeout.tv_sec += 1;
	}

	/* If "times" is zero, timeout_count is not touched, it is used by some
	 * machine states that can receive several timeout events without going
	 * to another state (e.g. probe). */

	if (times > 0) {
		timeout_count = times;
	}

	DEBUG("zeroconf timeout set to %d:%d, now is %d:%d",	timeout.tv_sec,
							 		timeout.tv_usec,
									now.tv_sec,
									now.tv_usec);
}

/* Disarms timeout bomb :) */

static void disarm_timeout(void)
{
	timeout.tv_sec = timeout.tv_usec = 0;
}

/* Timeout test functions */

static int is_timeout_disarmed(void)
{
	return timeout.tv_sec == 0 && timeout.tv_usec == 0;
}

/* Tests if timeout moment has passed (i.e. timeout is done) */

static int timeout_passed(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
		/* timeout disarmed */
		return 0;
	}

	return ((now.tv_sec > timeout.tv_sec) ||
	    (now.tv_sec == timeout.tv_sec && 
	     now.tv_usec >= timeout.tv_usec));
}

static int timeout_exhausted(void)
{
	return timeout_count <= 0;
}

/* Does the low-level disabling task, it is used by many functions */

static void _disable(void)
{
	close(zeroconf_fd);
	zeroconf_fd = -1;
	disarm_timeout();
}

/* State machine transition: disables zeroconf (probably because DHCP got an address) */

static void disable(void)
{
	DEBUG("zeroconf: state is Disabled.");
	_disable();
	state = ZEROCONF_SM_DISABLED;
}

static void disable_permanently(void)
{
	DEBUG("zeroconf: state is Disabled permanently.");
	_disable();
	state = ZEROCONF_SM_DISABLED_PERMANENTLY;
}

/* Sends ARP probe to see if our chosen IP address has another owner */

static void arp_probe(void)
{
	struct arpMsg arpbuffer;
	struct sockaddr iface;

	memset(&arpbuffer, 0, sizeof(arpbuffer));
	memcpy(arpbuffer.h_dest, MAC_BCAST_ADDR, 6);		/* MAC DA */
	memcpy(arpbuffer.h_source, mac_addr, 6);	/* MAC SA */
	arpbuffer.h_proto = htons(ETH_P_ARP);			/* protocol type (Ethernet) */
	arpbuffer.htype = htons(ARPHRD_ETHER);			/* hardware type */
	arpbuffer.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arpbuffer.hlen = 6;					/* hardware address length */
	arpbuffer.plen = 4;					/* protocol address length */
	arpbuffer.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	// arpbuffer.sInaddr = 0;				/* source IP address */
	memcpy(arpbuffer.sHaddr, mac_addr, 6);		/* source hardware address */
	memcpy(arpbuffer.tInaddr, &linklocal_addr, sizeof(linklocal_addr));	/* target IP address */

	memset(&iface, 0, sizeof(iface));
	strcpy(iface.sa_data, interface);

	DEBUG("zeroconf: Sending ARP probe.");

	if (sendto(zeroconf_fd, &arpbuffer, sizeof(arpbuffer), 0, &iface, sizeof(iface)) < 0) {

		/* serious error, disable zeroconf completely */

		perror("zeroconf: packet socket error in sendto(), disabling zeroconf.");

		disable_permanently();
	}
}

/* Announces the link-local address as broadcasted ARP "is-at" packets. */

static void arp_announce(struct in_addr *target)
{
	struct arpMsg arpbuffer;
	struct sockaddr iface;

	memset(&arpbuffer, 0, sizeof(arpbuffer));
	memcpy(arpbuffer.h_dest, MAC_BCAST_ADDR, 6);		/* MAC DA */
	memcpy(arpbuffer.h_source, mac_addr, 6);		/* MAC SA */
	arpbuffer.h_proto = htons(ETH_P_ARP);			/* protocol type (Ethernet) */
	arpbuffer.htype = htons(ARPHRD_ETHER);			/* hardware type */
	arpbuffer.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arpbuffer.hlen = 6;					/* hardware address length */
	arpbuffer.plen = 4;					/* protocol address length */
	arpbuffer.operation = htons(ARPOP_REPLY);		/* ARP op code */
	memcpy(arpbuffer.sInaddr, &linklocal_addr, sizeof(linklocal_addr));	/* source IP address */
	memcpy(arpbuffer.sHaddr, mac_addr, 6);			/* source hardware address */
	memcpy(arpbuffer.tInaddr, target, sizeof(struct in_addr));		/* target IP address */

	memset(&iface, 0, sizeof(iface));
	strcpy(iface.sa_data, interface);

	DEBUG("zeroconf: Sending ARP announcement.");

	if (sendto(zeroconf_fd, &arpbuffer, sizeof(arpbuffer), 0, &iface, sizeof(iface)) < 0) {

		/* serious error, disable zeroconf completely */

		perror("zeroconf: packet socket error in sendto(), disabling zeroconf.");

		disable_permanently();
	}
}

/* State machine transition: enables zeroconf (probably because DHCP didn't get a
 * better IP address yet */

static void initial_time(void)
{
	struct ifreq iface;
	int optval = 1;

	if (zeroconf_fd >= 0) {
		close(zeroconf_fd);
	}

	zeroconf_fd = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));

	if (zeroconf_fd < 0) {
		perror("zeroconf: link socket creation error, disabling zeroconf.");
		disable_permanently();
		return;
	}

	if (setsockopt(zeroconf_fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
		perror("zeroconf: Could not setsocketopt(BROADCAST) link socket");
		disable_permanently();
		return;
	}

	strncpy(iface.ifr_ifrn.ifrn_name, interface, IFNAMSIZ);
	if (setsockopt(zeroconf_fd, SOL_SOCKET, SO_BINDTODEVICE, (char*) &iface, sizeof(iface)) < 0) {
		perror("zeroconf: Could not setsocketopt(BINDTODEVICE) link socket");
		disable_permanently();
		return;
	}

	choose_addr();
	state = ZEROCONF_SM_INITIALTIME;

	/* Timeout is one-shot */
	set_timeout(ZEROCONF_PROBE_WAIT_MINIMUM + 
			  random() % (ZEROCONF_PROBE_WAIT_MAXIMUM - ZEROCONF_PROBE_WAIT_MINIMUM),
			  1);
}

/* State machine transition: after initial time, probe to see if the chosen link-local
 * address does not have an owner */

static void probe(void)
{
	if (state != ZEROCONF_SM_PROBE) {
		DEBUG("zeroconf: Changed state to PROBE");
		state = ZEROCONF_SM_PROBE;

		/* Timeout will fire for ZEROCONF_PROBE_NUM times */

		set_timeout(ZEROCONF_PROBE_MIN + 
					random() % (ZEROCONF_PROBE_MAX - ZEROCONF_PROBE_MIN),
				     ZEROCONF_PROBE_NUM);
		arp_probe();

	} else if (! timeout_exhausted()) {

		/* Timeout count not exausthed, probe again */
		
		DEBUG("zeroconf: PROBE state maintained, probing again");

		set_timeout(ZEROCONF_PROBE_MIN + 
					random() % (ZEROCONF_PROBE_MAX - ZEROCONF_PROBE_MIN),
				     0);
		arp_probe();
	} else {
		DEBUG("zeroconf: PROBE complete, going to active/ann state");
		active_announce();
	}
	
}

/* State machine transition: link-local address is ours, hooray! */

static void active_announce(void)
{
	if (state != ZEROCONF_SM_ACTIVE_ANNOUNCE) {
		struct dhcpMessage fake;

		DEBUG("zeroconf: state is Active/Announce, Link-local address %s",
				inet_ntoa(linklocal_addr));

		bzero(&fake, sizeof(fake));
		fake.yiaddr = linklocal_addr.s_addr;
		
		udhcp_run_script(&fake, "zeroconf");

		state = ZEROCONF_SM_ACTIVE_ANNOUNCE;
		
		/* Timeout will shoot ZEROCONF_ANNOUNCE_NUM times */
		set_timeout(ZEROCONF_ANNOUNCE_WAIT, ZEROCONF_ANNOUNCE_NUM);
		arp_announce(&linklocal_addr);

	} else if (! timeout_exhausted()) {
		DEBUG("zeroconf: Announcing again link-local address");
		set_timeout(ZEROCONF_ANNOUNCE_INTERVAL, 0);
		arp_announce(&linklocal_addr);

	} else {
		active_rest();
	}
}

/* Machine state: active and no more announcements due. Rest in peace */

static void active_rest(void)
{
	DEBUG("zeroconf: State is Active/rest");
	state = ZEROCONF_SM_ACTIVE_REST;
	conflict_count = 0;
	disarm_timeout();
}

/* Machine state: too many conflicts in link-local negotiation, sleep for a while */

static void rate_limit(void)
{
	DEBUG("zeroconf: State is Rate limit");

	_disable();
	state = ZEROCONF_SM_RATE_LIMIT;
	linklocal_addr.s_addr = 0;

	/* One-shot timeout */
	set_timeout(ZEROCONF_RATE_LIMIT_INTERVAL, 1);
}

/* Reads link-layer socket about ARP packets, and detects conflicts */

static int arp_conflict(void)
{
	struct arpMsg arpbuffer;
	
	if (zeroconf_fd < 0) {
		DEBUG("zeroconf: arp_conflict(): hey, socket is closed, do not call me!");
		return 0;
	}
	
	if (recv(zeroconf_fd, &arpbuffer, sizeof(arpbuffer), 0) < 0) {

		/* socket error, we disable zeroconf completely */

		perror("zeroconf: link socket read error, disabling zeroconf.");

		if (state == ZEROCONF_SM_ACTIVE_ANNOUNCE || state == ZEROCONF_SM_ACTIVE_REST) {
			udhcp_run_script(NULL, "deconfig");
		}
		disable_permanently();
		return 0;
	}

	if (arpbuffer.operation == htons(ARPOP_REPLY) && 
	    bcmp(arpbuffer.sHaddr, mac_addr, 6) != 0 &&
	    *((uint32_t *) arpbuffer.sInaddr) == linklocal_addr.s_addr) {

		/* if the packet is ARPOP_REPLY, and the source MAC address is not ours,
		 * and the source IP address *is* ours, there is a conflict. */
		
		DEBUG("zeroconf: conflict: %02x:%02x:%02x:%02x:%02x:%02x claims "
			      "to have same IP as ours.", arpbuffer.sHaddr[0], arpbuffer.sHaddr[1], 
			      arpbuffer.sHaddr[2], arpbuffer.sHaddr[3], arpbuffer.sHaddr[4],
			      arpbuffer.sHaddr[5]);
		return 1;
	}
	
	if (arpbuffer.operation == htons(ARPOP_REQUEST) && 
	    bcmp(arpbuffer.sHaddr, mac_addr, 6) != 0 &&
	    *((uint32_t *) arpbuffer.tInaddr) == linklocal_addr.s_addr) {

		/* if the packet is ARPOP_REQUEST, and the source MAC address is not ours,
		 * and the target IP addres *is* ours, send ARP reply as broadcast
		 * (ok, it will duplicate kernel response, a kernel patch for Zeroconf
		 * would be a better solution) */
		
		DEBUG("zeroconf: ARP request for our address, answering in broadcast.");
		arp_announce((struct in_addr*) &arpbuffer.sInaddr);
	}
	
	/* No conflict detected */

	return 0;
}

/* main state machine controller */

void zeroconf_event(int event)
{
	if (state == ZEROCONF_SM_PERMANENTLY_DISABLED) {
		DEBUG("zeroconf: event: we are permanently disabled, ignoring.");
		return;

	} else if (event == ZEROCONF_EVENT_DHCPIN) {
		/* DHCP offers a better address, cut off zeroconf */
		DEBUG("zeroconf: Going to disabled state.");
		disable();

	} else if (event == ZEROCONF_EVENT_DHCPOUT) {
		if (state == ZEROCONF_SM_DISABLED) {
			/* If zeroconf is off but not permanently off, turn it on */
			DEBUG("zeroconf: Going to 'initial time' state.");
			initial_time();
		}

	} else if (event == ZEROCONF_EVENT_SOCKETREADY) {
		if (arp_conflict() &&
		    state != ZEROCONF_SM_DISABLED && state != ZEROCONF_SM_DISABLED_PERMANENTLY) {
			DEBUG("zeroconf: Conflict detected and we are not disabled.");
			conflicted = 1;
			++conflict_count;
			if (conflict_count > ZEROCONF_MAX_COLLISIONS) {
				DEBUG("zeroconf: Too many conflicts, rate limiting!");
				rate_limit();
			} else {
				DEBUG("zeroconf: Back to square one.");
				initial_time();
			}
		}
	}

	/* We grab every opportunity to test if timeout has elapsed */

	if (timeout_passed()) {
		DEBUG("zeroconf: a timeout has passed");

		disarm_timeout();
		--timeout_count;

		if (state == ZEROCONF_SM_INITIALTIME) {
			DEBUG("zeroconf: initial time T/O, going to probe");
			probe();

		} else if (state == ZEROCONF_SM_PROBE) {
			DEBUG("zeroconf: probe T/O, probing again");
			probe();

		} else if (state == ZEROCONF_SM_ACTIVE_ANNOUNCE) {
			DEBUG("zeroconf: announce T/O, announcing again");
			active_announce();

		} else if (state == ZEROCONF_SM_RATE_LIMIT) {
			DEBUG("zeroconf: rate limit T/O, going to initial time");
			initial_time();
		}
	}
}

