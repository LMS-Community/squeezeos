#
# Makefile for the spi core.
#

obj-$(CONFIG_SPI)		+= spi-core.o
obj-y				+= busses/

ifeq ($(CONFIG_SPI_DEBUG_CORE),y)
EXTRA_CFLAGS += -DDEBUG
endif

