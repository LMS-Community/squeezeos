#
# (C) Copyright 2005 
# Samsung Electronics, Ryu Eui-Youl<steven.ryu@samsung.com>
#
# (C) Copyright 2000, 2001, 2002
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= lib$(CPU).a

ifeq ($(BOARD),smdk2442)
OBJS    = nand_cp_2442.o
else
	ifeq ($(BOARD),kingfish3)
	OBJS    = nand_cp_2442.o
	else
	OBJS    = nand_cp.o
	endif
endif

OBJS	+= serial.o interrupts.o cpu.o speed.o usb_ohci.o usb_ctl.o usb_recv.o usb_send.o usb_ep0.o

all:	.depend $(START) $(LIB)

$(LIB):	$(OBJS)
	$(AR) crv $@ $(OBJS)

#########################################################################

.depend:	Makefile $(START:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(START:.o=.S) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
