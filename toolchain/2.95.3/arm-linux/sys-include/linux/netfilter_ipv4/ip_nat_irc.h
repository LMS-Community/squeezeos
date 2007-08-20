#ifndef _IP_NAT_IRC_H
#define _IP_NAT_IRC_H
/* IRC extension for TCP NAT alteration.
 * (C) 2000 by Harald Welte <laforge@gnumonks.org>
 * based on RR's ip_nat_ftp.h
 *
 * ip_nat_irc.h,v 1.3 2000/09/14 12:22:13 laforge Exp
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *
 */


#ifndef __KERNEL__
#error Only in kernel.
#endif

/* Protects irc part of conntracks */
DECLARE_LOCK_EXTERN(ip_irc_lock);

/* We keep track of where the last SYN correction was, and the SYN
   offsets before and after that correction.  Two of these (indexed by
   direction). */
struct ip_nat_irc_info
{
	u_int32_t syn_correction_pos;                              
	int32_t syn_offset_before, syn_offset_after; 
};

#endif /* _IP_NAT_IRC_H */
