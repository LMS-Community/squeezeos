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

# sh3 support is untested... it is said to build, and "hello, world" works,
# but that's all I've heard.  
# FIXME: The sh3 is supposedly the same as an sh4
# but without the floating point unit, so maybe glibc has to be built
# --without-fp.  See powerpc-405.dat and
# http://www.gnu.org/software/libc/manual/html_node/Configuring-and-compiling.html#Configuring%20and%20compiling

#eval `cat sh3.dat gcc-3.3.2-glibc-2.3.2.dat` sh all.sh --notest
#eval `cat sh3.dat gcc-3.3.3-glibc-2.3.2.dat` sh all.sh --notest
#eval `cat sh3.dat gcc-3.4.0-glibc-2.3.2.dat` sh all.sh --notest
 eval `cat sh3.dat gcc-3.4.1-glibc-2.3.3.dat` sh all.sh --notest
#eval `cat sh3.dat gcc-3.4.1-glibc-20040827.dat` sh all.sh --notest

echo Done.
