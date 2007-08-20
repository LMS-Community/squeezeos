#
# Makefile for mini_fo 2.4 and 2.6 Linux kernels
#
# Copyright (C) 2004, 2005 Markus Klotzbuecher <mk@creamnet.de>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#

KERNELVERSION=$(shell uname -r | cut -d. -f1,2)

# Regular build, this should point to the kernel headers:
KERNEL_SRC = /lib/modules/$(shell uname -r)/build/
#KERNEL_SRC = /usr/src/linux-2.6.11.7/

TOPINC  = -I$(KERNEL_SRC)/include

# UML build, compile with "make ARCH=um"
UML_ROOTFS = /home/mk/uml/root_fs_slack8.1
UML_ROOTFS_MNT = /mnt/other/

MODDIR = /lib/modules/$(shell uname -r)

# allow custom override of TOPINC for fistgen developers
-include fistdev.mk

ifeq ($(KERNELVERSION),2.4)
CFLAGS = -D__KERNEL__ -DMODULE -DFISTGEN -I. ${TOPINC} -O2 -Wall -Wno-unused -g -fno-common -fno-schedule-insns -fno-schedule-insns2 -fno-strict-aliasing -msoft-float -Werror 
ifeq ($(ARCH),um)
CFLAGS += $(shell cd $(KERNEL_SRC) ; make script 'SCRIPT=@echo $$(CFLAGS)' ARCH=um)
endif
UCFLAGS = -DFISTGEN -I. ${TOPINC} -g -O2 -Wall -Wno-unused -Werror
endif

CC	= $(CROSS_COMPILE)gcc
LD	= $(CROSS_COMPILE)ld

obj-m := mini_fo.o
mini_fo-objs   := meta.o dentry.o file.o inode.o main.o super.o state.o aux.o

all: mini_fo$(KERNELVERSION)

mini_fo2.4: ${obj-m}

mini_fo2.6:
	make -C ${KERNEL_SRC} SUBDIRS=$(PWD) modules

${obj-m}: ${mini_fo-objs}
	$(LD) -o ${obj-m} -r ${mini_fo-objs}

tags:
	ctags -R -e

clean:
ifeq ($(KERNELVERSION),2.6)
	-make -C ${KERNEL_SRC} SUBDIRS=$(PWD) clean
endif
	rm -f ${obj-m} ${mini_fo-objs} ${obj-m:.o=.ko} *.d .*.flags *~
	rm -f TAGS

# dependencies
${mini_fo-objs}: mini_fo.h fist.h

install: mini_fo$(KERNELVERSION)
	@if [ -f "mini_fo.ko" ]; then					\
	  echo -e "\nInstalling mini_fo.ko to $(MODDIR)/kernel/fs/"    	;\
	  install -D -m 644 mini_fo.ko $(MODDIR)/kernel/fs/mini_fo.ko  	;\
	elif [ -f "mini_fo.o" ]; then					\
	  echo -e "\nInstalling mini_fo.o to $(MODDIR)/kernel/fs/" 	;\
	  install -D -m 644 mini_fo.o $(MODDIR)/kernel/fs/mini_fo.o    	;\
	fi								\

uninstall:
	@if [ -f $(MODDIR)/kernel/fs/mini_fo.ko ] && [ $(KERNELVERSION) == "2.6" ]; then									\
	  echo "Removing $(MODDIR)/kernel/fs/mini_fo.ko"	      ;\
	  rm $(MODDIR)/kernel/fs/mini_fo.ko			      ;\
	elif [ -f $(MODDIR)/kernel/fs/mini_fo.o ] && [ $(KERNELVERSION) == "2.4" ]; then								\
	  echo "Removing $(MODDIR)/kernel/fs/mini_fo.o"		      ;\
	  rm $(MODDIR)/kernel/fs/mini_fo.o			      ;\
	fi								\

umlinstall:
	@if ! mount|grep "$(UML_ROOTFS)"; then				 \
	  mount -o loop $(UML_ROOTFS) $(UML_ROOTFS_MNT)			;\
	fi
	@if [ -f "mini_fo.ko" ]; then			   	 	 \
	  echo "INSTALLING mini_fo.ko"					;\
	  rm $(UML_ROOTFS_MNT)/root/mini_fo.ko				;\
	  install -D -m 644 mini_fo.ko $(UML_ROOTFS_MNT)/root/mini_fo.ko ;\
	elif [ -f "mini_fo.o" ]; then			   	 	\
	  echo "INSTALLING mini_fo.o"					;\
	  rm $(UML_ROOTFS_MNT)/root/mini_fo.o				;\
	  install -D -m 644 mini_fo.o $(UML_ROOTFS_MNT)/root/mini_fo.o  ;\
	else								 \
	  echo "NO MODULE FOUND! BUILD FIRST?"				;\
	fi

	@umount $(UML_ROOTFS_MNT)				
