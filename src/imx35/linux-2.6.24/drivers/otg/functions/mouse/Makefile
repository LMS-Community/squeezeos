# Function driver for a Random Mouse
# @(#) balden@belcarra.com|otg/functions/mouse/Makefile-l26|20060419204257|16121
#
# Copyright (c) 2004 Belcarra
# Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp

mouse_fd-objs	:= mouse-fd.o
mouse_cf-objs	:= mouse-cf.o
mouse_if-objs	:= mouse-if.o

ifeq ($(CONFIG_OTG_MOUSE_TRADITIONAL),y)
obj-$(CONFIG_OTG_MOUSE) += mouse_fd.o
endif
ifeq ($(CONFIG_OTG_MOUSE_INTERFACE), y)
obj-$(CONFIG_OTG_MOUSE) += mouse_if.o
endif
ifeq ($(CONFIG_OTG_MOUSE_COMPOSITE), y)
obj-$(CONFIG_OTG_MOUSE) += mouse_cf.o
endif

OTG=$(TOPDIR)/drivers/otg
MOUSED=$(OTG)/functions/mouse
OTGCORE_DIR=$(OTG)/otgcore
#USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(MOUSED) -I$(OTG) -Wno-unused -Wno-format  -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(MOUSED) -I$(OTG) -Wno-unused -Wno-format  -I$(OTGCORE_DIR)
