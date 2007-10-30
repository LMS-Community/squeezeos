#
# Makefile for the L3 bus driver.
#

# Link order:
#  (core, adapters, algorithms, drivers) then clients

l3-$(CONFIG_L3_ALGOBIT)		+= l3-algo-bit.o
l3-$(CONFIG_BIT_SA1100_GPIO)	+= l3-bit-sa1100.o
l3-$(CONFIG_L3_BIT_S3C2413_GPIO)	+= l3-bit-s3c2413.o

obj-$(CONFIG_L3)		+= l3-core.o $(l3-y) $(l3-drv-y)
