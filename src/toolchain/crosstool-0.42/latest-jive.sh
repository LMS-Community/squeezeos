#!/bin/sh
set -ex
JIVE_PATH=`pwd`/../../..

TARBALLS_DIR=$HOME/crosstool/downloads
RESULT_TOP=$JIVE_PATH/toolchain
export TARBALLS_DIR RESULT_TOP
GCC_LANGUAGES="c,c++"
export GCC_LANGUAGES

#PARALLELMFLAGS=-j2
#export PARALLELMFLAGS

# Really, you should do the mkdir before running this,
# and chown /opt/crosstool to yourself so you don't need to run as root.
mkdir -p $RESULT_TOP

# Build the toolchain.  Takes a couple hours and a couple gigabytes.
eval `cat jive.dat latest.dat`  sh all.sh --notest -gdb

echo Done.
