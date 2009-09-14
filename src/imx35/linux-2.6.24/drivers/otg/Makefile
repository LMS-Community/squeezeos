#
# Belcarra OTG - On-The-Go
# @(#) balden@belcarra.com/seth2.rillanon.org|otg/.Makefiles/Makefile-freescale|20070530040553|49544
#
# Copyright (c) 2004-2005 Belcarra Technologies Corp
# Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp

TOPDIR ?= ../..

obj-y 		+=
obj-$(CONFIG_OTG) += otgcore/

EXTRA_CFLAGS += -Wno-format -Wall
# Function Drivers
obj-$(CONFIG_OTG_GENERIC) += functions/generic/
obj-$(CONFIG_OTG_ACM) += functions/acm/
obj-$(CONFIG_OTG_MSC) += functions/msc/
obj-$(CONFIG_OTG_MOUSE) += functions/mouse/
obj-$(CONFIG_OTG_NETWORK) += functions/network/

ifeq ($(CONFIG_OTG),m)
obj-m += hardware/
ifeq ($(CONFIG_OTG_NETWORK),m)
obj-y += functions/network/
endif
endif

ifeq ($(CONFIG_OTG),y)
obj-y += hardware/
endif
