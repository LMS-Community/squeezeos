#
# Belcarra OTG - On-The-Go
# @(#) sl@belcarra.com/whiskey.enposte.net|otg/otgcore/Makefile-l26|20061213004244|25170
#
# Copyright (c) 2004 Belcarra Technologies Corp
# Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
# Makefile for Linux 2.6.x systems

OTG=$(TOPDIR)/drivers/otg
EXTRA_CFLAGS +=          -Wno-missing-prototypes -Wno-unused -Wno-format -I$(OTG)
EXTRA_CFLAGS_nostdinc += -Wno-missing-prototypes -Wno-unused -Wno-format -I$(OTG)

otgcore-objs		:= core-init-lnx.o otg.o \
			otg-trace.o otg-trace-lnx.o \
			otg-mesg.o otg-mesg-lnx.o \
			otg-fw.o usbp-bops.o usbp-fops.o usbp-procfs.o

ifeq ($(CONFIG_OTG_USB_PERIPHERAL),y)
otgcore-objs		+= otg-fw-mn.o
endif

ifeq ($(CONFIG_OTG_USB_HOST),y)
otgcore-objs		+= otg-fw-mn.o
endif

ifeq ($(CONFIG_OTG_USB_PERIPHERAL_OR_HOST),y)
otgcore-objs		+= otg-fw-mn.o
endif

ifeq ($(CONFIG_OTG_BDEVICE_WITH_SRP),y)
otgcore-objs		+= otg-fw-df.o
endif

ifeq ($(CONFIG_OTG_DEVICE),y)
otgcore-objs		+= otg-fw-df.o
endif


#ifeq ("$(CONFIG_OTG_TRACE)", "y")
#otgcore-objs		+= usbp-procfs.o
#endif

#usbprocfs-objs		:= usbp-procfs.o

obj-$(CONFIG_OTG)	+= otgcore.o

#obj-$(CONFIG_OTG_PROCFSM)       += usbprocfs.o

#otgtrace-objs		:= otg-trace.o
#obj-$(CONFIG_OTG_TRACE)	+= otgtrace.o
