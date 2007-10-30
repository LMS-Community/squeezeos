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

#eval `cat arm.dat gcc-2.95.3-glibc-2.1.3.dat` sh all.sh --notest
#eval `cat arm.dat gcc-2.95.3-glibc-2.2.2.dat` sh all.sh --notest
#eval `cat arm.dat gcc-3.2.3-glibc-2.2.5.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.3-glibc-2.2.5.dat`    sh all.sh --notest
#eval `cat arm.dat gcc-3.3-glibc-2.3.2.dat`    sh all.sh --notest
#eval `cat arm.dat gcc-3.3.1-glibc-2.3.2.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.3.2-glibc-2.3.2.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.3.3-glibc-2.3.2.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.3.4-glibc-2.3.2.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.3.4-glibc-2.3.3.dat`  sh all.sh --notest --testlinux
#eval `cat arm.dat gcc-3.3.6-glibc-2.3.5.dat`  sh all.sh --notest --testlinux
#eval `cat arm.dat gcc-3.4.0-glibc-2.3.2.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.4.1-glibc-2.3.2.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.4.1-glibc-2.3.3.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.4.2-glibc-2.2.5.dat`  sh all.sh --notest --testlinux
#eval `cat arm.dat gcc-3.4.2-glibc-2.3.3.dat`  sh all.sh --notest
#eval `cat arm.dat gcc-3.4.1-glibc-20040827.dat`  sh all.sh --notest
 eval `cat arm.dat gcc-4.1-20050709-glibc-2.3.2.dat`  sh all.sh --notest

echo Done.
