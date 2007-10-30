#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
# David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
#
# SAMSUNG SMDK2410 board with S3C2410X (ARM920T) cpu
#
# see http://www.samsung.com/ for more information on SAMSUNG
#

#
# SMDK2410 has 1 bank of 64 MB DRAM
#
# 3000'0000 to 3400'0000
#
# Linux-Kernel is expected to be at 3000'8000, entry 3000'8000
# optionally with a ramdisk at 3080'0000
#
# we load ourself to 33F8'0000
#
# download area is 3300'0000
#

#Select one of TEXT_BASE
TEXT_BASE = 0x11E00000		# for 496 PKG (defualt)
#TEXT_BASE = 0x21E00000		# for 416 PKG & DDR
#TEXT_BASE = 0x23E00000		# for 416 PKG & SDRAM
S3C24X0_NAND = s3c2460x_nand
