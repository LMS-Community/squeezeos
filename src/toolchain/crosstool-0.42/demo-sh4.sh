#!/bin/sh
set -ex
TARBALLS_DIR=$HOME/downloads
RESULT_TOP=/opt/crosstool
export TARBALLS_DIR RESULT_TOP
GCC_LANGUAGES="c,c++"
export GCC_LANGUAGES

# Really, you should do the mkdir before running this,
# and chown /opt/crosstool to yourself so you don't need to run as root.
mkdir -p $RESULT_TOP

# Build the toolchain.  Takes a couple hours and a couple gigabytes.
# note: binutils < 2.13 doesn't know about sh4, so don't try building gcc-2.95
# note: gcc-3.2.3 has ICE compiling glibc for sh4 (http://gcc.gnu.org/PR6954), so don't try building gcc-3.2.3

#eval `cat sh4.dat gcc-3.3-glibc-2.2.5.dat`    sh all.sh --notest
#eval `cat sh4.dat gcc-3.3-glibc-2.3.2.dat`    sh all.sh --notest 
#eval `cat sh4.dat gcc-3.3.2-glibc-2.3.2.dat`  sh all.sh --notest 
#eval `cat sh4.dat gcc-3.3.3-glibc-2.3.2.dat`  sh all.sh --notest 
#eval `cat sh4.dat gcc-3.4.0-glibc-2.3.2.dat`  sh all.sh --notest 
#eval `cat sh4.dat gcc-3.4.1-glibc-2.3.3.dat`  sh all.sh --notest 
#eval `cat sh4.dat gcc-3.4.1-glibc-20040827.dat`  sh all.sh --notest 
#eval `cat sh4.dat gcc-4.0.0-glibc-2.3.5.dat`  sh all.sh --notest 
 eval `cat sh4.dat gcc-4.1-20050716-glibc-2.3.2.dat` sh all.sh --notest --testlinux

echo Done.
