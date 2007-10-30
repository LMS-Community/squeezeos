#!/bin/sh
set -ex
# Little-endian MIPS
#
# To create a toolchain for the Linksys wrt54g, select glibc-2.2.3
# See http://seattlewireless.net/index.cgi/LinksysWrt54g
#     http://www.batbox.org/wrt54g-linux.html
# Note: recent wrt54g firmware uses uclibc, which behaves like a subsetted glibc.
# There are patches to build uclibc toolchains in the contrib directory,
# but they're not integrated yet.  However, you can still use a glibc
# toolchain; you'll either have to 
#  a) not call the missing functions,
#  b) use a stub library like http://www.xse.com/leres/tivo/downloads/libtivohack/
# or c) link your programs statically if you want them to run on
# the wrt54g.

TARBALLS_DIR=$HOME/downloads
RESULT_TOP=/opt/crosstool
export TARBALLS_DIR RESULT_TOP
GCC_LANGUAGES="c,c++"
export GCC_LANGUAGES

# Really, you should do the mkdir before running this,
# and chown /opt/crosstool to yourself so you don't need to run as root.
mkdir -p $RESULT_TOP

# Build the toolchain.  Takes a couple hours and a couple gigabytes.
#eval `cat mipsel.dat gcc-3.2.3-glibc-2.2.3.dat`        sh all.sh --notest
#eval `cat mipsel.dat gcc-3.2.3-glibc-2.2.5.dat`        sh all.sh --notest
#eval `cat mipsel.dat gcc-3.3-glibc-2.3.2.dat`          sh all.sh --notest
#eval `cat mipsel.dat gcc-3.3.2-glibc-2.3.2.dat`        sh all.sh --notest
#eval `cat mipsel.dat gcc-3.3.3-glibc-2.3.2.dat`        sh all.sh --notest
#eval `cat mipsel.dat gcc-3.4.0-glibc-2.3.2.dat`        sh all.sh --notest
#eval `cat mipsel.dat gcc-3.4.1-glibc-2.2.5.dat`        sh all.sh --notest
#eval `cat mipsel.dat gcc-3.4.1-glibc-2.3.2.dat`        sh all.sh --notest
 eval `cat mipsel.dat gcc-3.4.2-glibc-2.2.5.dat`        sh all.sh --notest
#eval `cat mipsel.dat gcc-3.4.2-glibc-2.3.3.dat`        sh all.sh --notest
#eval `cat mipsel.dat gcc-3.4.2-glibc-20040827.dat`        sh all.sh --notest

echo Done.
