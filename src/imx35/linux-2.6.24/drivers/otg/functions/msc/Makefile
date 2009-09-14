# Function driver for a Random Mouse
# @(#) balden@belcarra.com|otg/functions/msc/Makefile-l26|20060419204258|01484
#
# Copyright (c) 2004 Belcarra
# Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp

msc_if-objs	:= msc-fd.o crc.o msc-linux.o msc-bo.o msc-io-l24.o


obj-$(CONFIG_OTG_MSC) += msc_if.o

OTG=$(TOPDIR)/drivers/otg
ACMD=$(OTG)/functions/msc
OTGCORE_DIR=$(OTG)/otgcore
USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR) -I$(OTGCORE_DIR)
