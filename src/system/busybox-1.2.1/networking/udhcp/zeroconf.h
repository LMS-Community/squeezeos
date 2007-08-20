/* dhcpc.h */
#ifndef _ZEROCONF_H
#define _ZEROCONF_H

#include <sys/time.h>

#define ZEROCONF_SM_PERMANENTLY_DISABLED -1
#define ZEROCONF_SM_DISABLED_PERMANENTLY ZEROCONF_SM_PERMANENTLY_DISABLED
#define ZEROCONF_SM_DISABLED 0
#define ZEROCONF_SM_INITIALTIME 1
#define ZEROCONF_SM_PROBE 2
#define ZEROCONF_SM_ACTIVE_ANNOUNCE 4
#define ZEROCONF_SM_ACTIVE_REST 5
#define ZEROCONF_SM_ACTIVE_DEFENDING 6
#define ZEROCONF_SM_RATE_LIMIT 7

#define ZEROCONF_EVENT_TICK 1
#define ZEROCONF_EVENT_DHCPIN 2
#define ZEROCONF_EVENT_DHCPOUT 3
#define ZEROCONF_EVENT_START ZEROCONF_EVENT_DHCP_OUT
#define ZEROCONF_EVENT_SOCKETREADY 4

extern int zeroconf_fd;

void zeroconf_init(int enable, uint8_t arp[], const char* interface, int ifindex);
void zeroconf_event(int event);
void zeroconf_set_smallest_timeout(struct timeval* tm, long int other_timeout);

/* Zeroconf constants, time constants in microseconds */

#define ZEROCONF_PROBE_WAIT_MINIMUM	 500*1000
#define ZEROCONF_PROBE_WAIT_MAXIMUM	1000*1000
#define ZEROCONF_PROBE_MIN 		 200*1000
#define ZEROCONF_PROBE_MAX		 400*1000
#define ZEROCONF_PROBE_NUM		3
#define ZEROCONF_ANNOUNCE_WAIT		1000*1000
#define ZEROCONF_ANNOUNCE_NUM		2
#define ZEROCONF_ANNOUNCE_INTERVAL	2000*1000
#define ZEROCONF_MAX_COLLISIONS		10
#define ZEROCONF_RATE_LIMIT_INTERVAL	60*1000*1000
#define ZEROCONF_DEFEND_INTERVAL	10*1000*1000

#endif

