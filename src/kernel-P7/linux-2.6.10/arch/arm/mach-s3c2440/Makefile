
#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y			:= cpu.o irq.o time.o gpio.o clock.o devs.o
obj-m			:=
obj-n			:=
obj-			:=

# S3C2440 support files

obj-$(CONFIG_CPU_S3C2440)  += s3c2440.o
obj-$(CONFIG_S3C2440_DMA)  += dma.o
obj-$(CONFIG_S3C2440_DMA_SYSFS)  += dma-sysfs.o

# Power Management support

obj-$(CONFIG_PM)	   += pm.o sleep.o

# S3C2440 support

obj-$(CONFIG_CPU_S3C2440)  += s3c2440.o s3c2440-dsc.o

# machine specific support

obj-$(CONFIG_ARCH_BAST)		+= mach-bast.o usb-simtec.o
obj-$(CONFIG_ARCH_H1940)	+= mach-h1940.o
obj-$(CONFIG_ARCH_SMDK2440)	+= mach-smdk2440.o
obj-$(CONFIG_SIMTEC_AUDIO)    += audio-simtec.o

obj-$(CONFIG_MACH_VR1000)	+= mach-vr1000.o usb-simtec.o
obj-$(CONFIG_MACH_RX3715)	+= mach-rx3715.o
