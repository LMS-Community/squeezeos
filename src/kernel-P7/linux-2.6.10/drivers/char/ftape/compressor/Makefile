#
#       Copyright (C) 1997 Claus-Justus Heine.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Source: /home/cvs/s3c2413_kernel2.6.10_dev/drivers/char/ftape/compressor/Makefile,v $
# $Revision: 1.1.1.1 $
# $Date: 2005/11/16 00:54:56 $
#
#      Makefile for the optional compressor for th zftape VFS
#      interface to the QIC-40/80/3010/3020 floppy-tape driver for
#      Linux.
#

obj-$(CONFIG_ZFT_COMPRESSOR) += zft-compressor.o

zft-compressor-objs := zftape-compress.o lzrw3.o

CFLAGS_lzrw3.o	:= -O6 -funroll-all-loops
