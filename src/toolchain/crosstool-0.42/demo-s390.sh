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

# fails with "soinit.c:25: internal compiler error: in named_section_flags, at varasm.c:412"
# see http://gcc.gnu.org/PR9552
#eval `cat s390.dat gcc-3.3-glibc-2.3.2.dat`    sh all.sh --notest
# fails with "chown.c:65: `__libc_missing_32bit_uids' undeclared"
# maybe there's a patch for this in Debian's glibc combo patch,
# http://security.debian.org/debian-security/pool/updates/main/g/glibc/glibc_2.2.5-11.5.diff.gz
#eval `cat s390.dat gcc-3.2.3-glibc-2.2.5.dat`  sh all.sh --notest
#eval `cat s390.dat gcc-3.3-20040112-glibc-2.3.2.dat` sh all.sh --notest

# But this works, hurrah!
#eval `cat s390.dat gcc-3.4.0-glibc-2.3.2.dat`  sh all.sh --notest 
 eval `cat s390.dat gcc-3.4.1-glibc-2.3.3.dat`  sh all.sh --notest 
#eval `cat s390.dat gcc-3.4.1-glibc-20040827.dat` sh all.sh --notest

echo Done.
