#
# Network Function Driver
# @(#) balden@belcarra.com/seth2.rillanon.org|otg/functions/network/Makefile-l26|20070509205304|55506
#
# Copyright (c) 2002-2005 Belcarra Technologies Corp
# Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp

network_if-objs	:= net-l24-os.o net-fd.o blan-if.o basic-if.o basic2-if.o ecm-if.o eem-if.o safe-if.o fermat.o net-ip.o

obj-$(CONFIG_OTG_NETWORK) += network_if.o

obj-y += net-l24-fix.o

OTG=$(TOPDIR)/drivers/otg
NETWORKD=$(OTG)/functions/network
OTGCORE_DIR=$(OTG)/otgcore
EXTRA_CFLAGS += -I$(NETWORKD) -I$(OTG) -Wno-unused -Wno-format  -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(NETWORKD) -I$(OTG) -Wno-unused -Wno-format  -I$(OTGCORE_DIR)



# Link rules for multi-part drivers.

#network_fd.o: $(network_fd-objs)
#	$(LD) -r -o $@ $(network_fd-objs)
