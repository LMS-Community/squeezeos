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

 eval `cat cris.dat gcc-3.2.3-glibc-2.2.5.dat`  sh all.sh --notest
#eval `cat cris.dat gcc-3.3.4-glibc-2.2.5.dat`  sh all.sh --notest
#eval `cat cris.dat gcc-3.4.1-glibc-2.2.5.dat`  sh all.sh --notest
# cris doesn't build with glibc-2.3.2;
# fails with "errno-loc.c:39: error: `pthread_descr' undeclared" in glibc build.
# The cris glibc maintainer is aware of the problem and hopes to fix this later in 2004.
#eval `cat cris.dat gcc-3.3-glibc-2.3.2.dat`  sh all.sh --notest
#eval `cat cris.dat gcc-3.3.1-glibc-2.3.2.dat` sh all.sh --notest

echo Done.
