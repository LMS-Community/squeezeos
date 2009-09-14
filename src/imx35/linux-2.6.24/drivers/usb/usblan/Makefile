#
# Makefile for USB Network drivers
#

OTGDIR=$(srctree)/drivers/otg
EXTRA_CFLAGS += -I$(OTGDIR)
EXTRA_CFLAGS_nostdinc += -I$(OTGDIR)
obj-$(CONFIG_USB_USBLAN)	+= usblan.o
