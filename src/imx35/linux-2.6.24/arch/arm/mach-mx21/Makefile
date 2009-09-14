#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y			:= mm.o time.o dma.o gpio_mux.o clock.o devices.o serial.o system.o
obj-$(CONFIG_MACH_MX21ADS)	+= mx21ads.o mx21ads_gpio.o

# power management
obj-$(CONFIG_PM) 		+= pm.o mxc_pm.o
obj-$(CONFIG_DPM)		+= dpm.o
