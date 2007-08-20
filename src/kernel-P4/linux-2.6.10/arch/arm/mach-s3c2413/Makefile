
#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y			:= cpu.o irq.o time.o gpio.o clock.o devs.o
obj-m			:=
obj-n			:=
obj-			:=

# S3C2440 support files

obj-$(CONFIG_CPU_S3C2413)  += s3c2413.o
obj-$(CONFIG_S3C2413_DMA)  += dma.o
obj-$(CONFIG_S3C2413_DMA_SYSFS)  += dma-sysfs.o

# Power Management support

obj-$(CONFIG_PM)	   += pm.o sleep.o

# S3C2440 support

obj-$(CONFIG_CPU_S3C2413)  += s3c2413.o s3c2413-dsc.o

# machine specific support

obj-$(CONFIG_ARCH_SMDK2413)	+= mach-smdk2413.o

