#
# Makefile for the WatchDog device drivers.
#

# Only one watchdog can succeed. We probe the hardware watchdog
# drivers first, then the softdog driver.  This means if your hardware
# watchdog dies or is 'borrowed' for some reason the software watchdog
# still gives you some cover.

obj-$(CONFIG_JIVE_MATRIX) += jive_matrix.o
obj-$(CONFIG_JIVE_MGMT) += jive_mgmt.o
obj-$(CONFIG_JIVE_WHEEL) += jive_wheel.o
obj-$(CONFIG_JIVE_IRTX) += jive_irtx.o
