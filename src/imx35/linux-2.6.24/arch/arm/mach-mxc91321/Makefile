#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y := devices.o iomux.o system.o cpu.o mm.o clock.o serial.o dma.o dptc.o
obj-$(CONFIG_MACH_MXC30030EVB)		+= mxc30030evb.o mxc30030evb_gpio.o
obj-$(CONFIG_MACH_MXC30030ADS)		+= mxc30030ads.o mxc30030ads_gpio.o

# Dynamic power management
obj-$(CONFIG_PM)		+= pm.o mxc_pm.o