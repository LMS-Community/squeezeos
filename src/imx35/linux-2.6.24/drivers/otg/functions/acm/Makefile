# Function driver for a CDC ACM OTG Device
# @(#) balden@belcarra.com|otg/functions/acm/Makefile-l26|20060419204257|06816
#
# Copyright (c) 2004 Belcarra Technologies Corp
# Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp

tty_if-objs	:= acm-l26.o  acm.o tty-l26-os.o tty-if.o

obj-$(CONFIG_OTG_ACM) += tty_if.o

OTG=$(TOPDIR)/drivers/otg
ACMD=$(OTG)/functions/acm
USBDCORE_DIR=$(OTG)/usbdcore
EXTRA_CFLAGS += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR)
EXTRA_CFLAGS_nostdinc += -I$(ACMD) -I$(OTG) -Wno-unused -Wno-format  -I$(USBDCORE_DIR)
