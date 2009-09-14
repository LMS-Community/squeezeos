# File: Makefile
#
# (c) Copyright © 2003-2006, Marvell International Ltd. 
#
# This software file (the "File") is distributed by Marvell International 
# Ltd. under the terms of the GNU General Public License Version 2, June 1991 
# (the "License").  You may use, redistribute and/or modify this File in 
# accordance with the terms and conditions of the License, a copy of which 
# is available along with the File in the gpl.txt file or by writing to 
# the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
# 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
#
# THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
# ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
# this warranty disclaimer.

CC=		$(CROSS_COMPILE)gcc
LD=		$(CROSS_COMPILE)ld


#############################################################################
# Configuration Options
#############################################################################

# Debug Option
# DEBUG LEVEL n/1/2:
# n: NO DEBUG
# 1: Only PRINTM(MSG,...), PRINTM(FATAL,...), ...
# 2: All PRINTM()
CONFIG_DEBUG=1

# Proc in /proc/net/wlan
CONFIG_PROC_DEBUG=y

# Linux Power Management
CONFIG_ENABLE_PM=n

# Re-association in driver
CONFIG_REASSOCIATION=y

# Manufacturing firmware support
CONFIG_MFG_CMD_SUPPORT=y

#############################################################################
# Select Platform Tools
#############################################################################

MODEXT = ko
EXTRA_CFLAGS += -I$(PWD)/os/linux
EXTRA_CFLAGS += -I$(PWD)/wlan

#TOOLPATH_BULVERDE=	/usr/local/arm/3.3.2/bin
BULVERDE_DIR= 		/tftpboot/bulverde/root

KERNELDIR=/home/richard/imx35/linux-2.6.24/

EXTRA_CFLAGS += -I$(PWD)/if/if_sdio

#############################################################################
# Compiler Flags
#############################################################################

	EXTRA_CFLAGS += -I$(KERNELDIR)/include

	EXTRA_CFLAGS += -DFPNUM='"4"'

ifeq ($(CONFIG_DEBUG),1)
	EXTRA_CFLAGS += -DDEBUG_LEVEL1
endif

ifeq ($(CONFIG_DEBUG),2)
	EXTRA_CFLAGS += -DDEBUG_LEVEL1
	EXTRA_CFLAGS += -DDEBUG_LEVEL2
	DBG=	-dbg
endif

ifeq ($(CONFIG_PROC_DEBUG),y)
	EXTRA_CFLAGS += -DPROC_DEBUG
	export CONFIG_PROC_DEBUG
endif

ifeq ($(CONFIG_ENABLE_PM),y)
	EXTRA_CFLAGS += -DENABLE_PM
endif

ifeq ($(CONFIG_REASSOCIATION),y)
	EXTRA_CFLAGS += -DREASSOCIATION
endif

ifeq ($(CONFIG_MFG_CMD_SUPPORT),y)
	EXTRA_CFLAGS += -DMFG_CMD_SUPPORT
endif

#############################################################################
# Make Targets
#############################################################################

ifneq ($(KERNELRELEASE),)

WLANOBJS += wlan/wlan_11d.o
WLANOBJS += wlan/wlan_cmd.o
WLANOBJS += wlan/wlan_cmdresp.o
WLANOBJS += wlan/wlan_fw.o
WLANOBJS += wlan/wlan_join.o
WLANOBJS += wlan/wlan_main.o
WLANOBJS += wlan/wlan_rx.o
WLANOBJS += wlan/wlan_scan.o
WLANOBJS += wlan/wlan_tx.o
WLANOBJS += wlan/wlan_wext.o
WLANOBJS += wlan/wlan_wmm.o

ifdef CONFIG_PROC_FS
WLANOBJS += wlan/wlan_proc.o
ifeq ($(CONFIG_PROC_DEBUG), y)
WLANOBJS += wlan/wlan_debug.o
endif
endif

IFOBJS := if/if_sdio/if_sdio.o
obj-m := sd8686.o
sd8686-objs := $(WLANOBJS) $(IFOBJS)

# Otherwise we were called directly from the command line; invoke the kernel build system.
else

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

###############################################################

export		CC LD EXTRA_CFLAGS KERNELDIR CROSS_COMPILE

.PHONY: clean distclean

	@echo "Finished Making Marvell Wlan Linux Driver"

wlan app/wlanconfig:
	$(MAKE) -C $@

clean:
	-find . -name "*.o" -exec rm {} \;
	-find . -name "*.ko" -exec rm {} \;
	-find . -name ".*.cmd" -exec rm {} \;
	-find . -name "*.mod.c" -exec rm {} \;
	-rm -rf .tmp_versions
	$(MAKE) -C app/wlanconfig $@

distclean:
	-find . -name "*.o" -exec rm {} \;
	-find . -name "*.orig" -exec rm {} \;
	-find . -name "*.swp" -exec rm {} \;
	-find . -name "*.*~" -exec rm {} \;
	-find . -name "*~" -exec rm {} \;
	-find . -name "*.d" -exec rm {} \;
	-find . -name "*.a" -exec rm {} \;
	-find . -name "tags" -exec rm {} \;
	-find . -name ".*" -exec rm -rf 2> /dev/null \;
	-find . -name "*.ko" -exec rm {} \;
	-find . -name ".*.cmd" -exec rm {} \;
	-find . -name "*.mod.c" -exec rm {} \;
	-rm -rf .tmp_versions
	$(MAKE) -C app/wlanconfig $@

# End of file
