# Generic Composite Driver
#
# @(#) balden@belcarra.com|otg/functions/generic/Makefile-l26|20060419204257|43174
# Copyright (c) 2004-2005 Belcarra Technologies Corp
# Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp

ifeq ($(CONFIG_OTG_GENERIC),m)
generic_cf-objs         := generic-cf.o generic.o
else
generic_cf-objs         := generic-cf.o
obj-y                   += generic.o
endif

extra-y			:= generic.o


generic_cl-objs         := generic-cl.o

obj-$(CONFIG_OTG_GENERIC) += generic_cf.o
obj-$(CONFIG_OTG_GENERIC) += generic_cl.o


OTG=$(TOPDIR)/drivers/otg
OTGCORE_DIR=$(OTG)/otgcore
EXTRA_CFLAGS += -I$(OTG) -Wno-unused -Wno-format  -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(OTG) -Wno-unused -Wno-format -I$(OTGCORE_DIR)
