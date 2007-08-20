#
# Makefile for the spi bus drivers.
#


obj-$(CONFIG_SPI_S3C2413)	+= s3c2413-spi.o

ifeq ($(CONFIG_SPI_DEBUG_BUS),y)
EXTRA_CFLAGS += -DDEBUG
endif

