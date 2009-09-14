#!/bin/sh

# Temporary Variable for easy testing
DEBUG=off

#
# Mostly static variables, but could change in the future
#
STARTDIR=$PWD
WORKDIR=$STARTDIR/tmp/work/
ROOTFS=$STARTDIR/tmp/rootfs/
OUTPUTPREFIX=$STARTDIR/squeezecenter+perl-$TARGET-r

# 
# Make sure that $TARGET was supplied
#
if [ "$TARGET" = "" ]; then 
	echo "Please set the \$TARGET variable to the architecture of the platform your building for."
	exit 1
fi

#
# Packages to get rid of
#
PACKAGES="binutils freefont freetype gcc-cross glibc glibc-intermediate jivetest libtool-cross linux-libc-headers lua luafilesystem luajson lualogging lualoop luamd5 luaprofiler luasocket luasyslog luatolua++ luazipfilter lzo marvell-*-* mtd-utils ncurses readline squeezeplay tremor uboot-env wireless-tools"

#
# Now for each package lets get a list of the files that it puts into its /image dir. From there we'll have a list of files to remove
# from the /rootfs/ dir
#
for package in $PACKAGES
do
	#
	# Set up some easy to use variables to make sure we know where we're looking, 
	# and where we're deleting from.
	#
	source="$WORKDIR/$TARGET/$package-*/image/"

	echo "INFO: Beginning package [$package] by searching:"
	echo "INFO:		$source"

	#
	# Now, do a find in the directory listed above. For every file we find up there, see if it exists in the 
	# $ROOTFS dir. If it does, remove it
	#
	FILESTOREMOVE=`find $source -type f | awk -F/image/ '{print $2}'` 
	if [ "$DEBUG" = "on" ]; then echo "DEBUG: Here's the list of files we found: $FILESTOREMOVE"; fi
	
	#
	# Now start a loop for each file that was listed...
	#
	for file in $FILESTOREMOVE
	do
		if [ -e $ROOTFS/$file ]; then
			rm $ROOTFS/$file
		else
			if [ "$DEBUG" = "on" ]; then echo "DEBUG: Did not find $ROOTFS/$file, not running rm."; fi
		fi
	done		
done

#
# Now we know we need to remove some other things that are just not quite listed in packages
#
rm -rf $ROOTFS/dev $ROOTFS/etc $ROOTFS/lib/modules $ROOTFS/lib/firmware

#
# Now that we've cleaned up the $ROOTFS, lets tar it up
#
REV=`grep rev $ROOTFS/usr/squeezecenter/build.txt | awk -F' ' '{print $2}'`
OUTPUT=$OUTPUTPREFIX$REV".tgz"
cd $ROOTFS

echo "INFO: Running [cleanlinks] to remove any left over links or directories"
cleanlinks

echo "INFO: Creating tarball now named [$OUTPUT]"
tar -czf $OUTPUT .
cd $STARTDIR

echo "INFO: Here are the files that went into [$OUTPUT]"
tar -ztvf $OUTPUT
