#!/bin/sh
#
# Copyright 2004 Google Inc.
# All Rights Reserved.  This script is provided under the terms of the GPL.
#
# Author: matthewbg@google.com (Matthew Beaumont-Gay)
#
# Creates the support library override directories and associated scripts

abort() {
  echo $@
  exec false
}

test -z "${TARGET}" && abort "Please set TARGET to the Gnu target identifier (e.g. pentium-linux)"
test -z "${PREFIX}" && abort "Please set PREFIX to where you want the toolchain installed."

set -ex

installOneLib() {
  # Make a unique directory for each shared library named after the file,
  # but with .dir appended.
  FILE=$1
  NEWDIR=$FILE.dir
  rm -rf $NEWDIR
  mkdir -p $NEWDIR

  # Install the shared library in its private little directory
  # Use *hard* link to save space, since some shared libraries are quite large,
  # and since ldconfig ignores symbolic links.
  ln $FILE $NEWDIR

  # Create little scripts in that directory to install and uninstall
  # the directory in /etc/ld.so.conf
  cat >$NEWDIR/install.sh <<_EOF_
#!/bin/sh
sh $PREFIX/libexec/install-shared-lib.sh $NEWDIR
_EOF_
  cat >$NEWDIR/uninstall.sh <<_EOF_
#!/bin/sh
sh $PREFIX/libexec/install-shared-lib.sh --uninstall $NEWDIR
_EOF_
}

# These should match the lib* subpackages in the crosstool-gcc specfile
# FIXME: Include more shared libraries, like java and fortran
LIBS="libgcc_s.so libstdc++.so"

for LIB in $LIBS; do
  # We don't know where the library is, so search.
  # Filter out links installed by this script using grep...
  FILES=`find $PREFIX/$TARGET/lib* -name "$LIB*" -type f | grep -v '\.dir$' || true `
  # We expect only one match, but what the heck, if both /lib and /lib64 exist, do 'em both?
  for FILE in $FILES; do
     installOneLib $FILE
  done
done

# Oh, what the heck, also install the script :-)
install -D -m755 install-shared-lib.sh $PREFIX/libexec/install-shared-lib.sh

