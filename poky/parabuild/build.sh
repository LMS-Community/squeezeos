#!/bin/bash

CPUCOUNT=`cat /proc/cpuinfo | grep processor | wc -l`

# Figure out how many CPUs we have, and set our multithreading
# options accordingly. 
CPUCOUNT=`cat /proc/cpuinfo | grep processor | wc -l`
DOUBLECPUCOUNT=`echo "$CPUCOUNT * 2" | bc`

# Function to make sure we call bitbake the same way everywhere
function run_bitbake {
	bitbake -vvv $*
}

echo "*** Setting poky environment ***"
source poky-init-build-env

if [ -f $PARABUILD_ENV_SCRIPT ]
then
	source $PARABUILD_ENV_SCRIPT
fi

echo "*** Clean cache and squeezeplay ***"

# Clean the cache to ensure poky checks out updated packages from SVN
rm -rf tmp-${MACHINE}/cache
rm -rf tmp/cache

# Clean squeezeplay to ensure that the firmware revision number is correct
run_bitbake "squeezeplay -c clean" > /dev/null

echo "*** Update the local.conf file ***"
if [ "x${SQUEEZEOS_PRIVATE_SVN}" != "x" ]
then
	echo "*** Enabled private svn ***"

	# Enable the private modules (wlan drivers, audio codecs, etc)
	sed -e 's/##//' conf/local.conf.sample > conf/local.conf

	# Set the SVN path for the private modules
	echo "SQUEEZEOS_PRIVATE_SVN = \"${SQUEEZEOS_PRIVATE_SVN}\"" >> conf/local.conf
else
	cp conf/local.conf.sample conf/local.conf
fi

# Check if a machine type exists
if [ "x${MACHINE}" != "x" ]
then
	echo "*** Building for MACHINE type: ${MACHINE}"
else
	echo "ERROR: No MACHINE specified!"
	exit 1
fi

echo "INHERIT += \"rm_work\"" >> conf/local.conf

# Make build use all available CPUs
(
	# Disable all parallel build while trying to isolate problem
	
	# echo BB_NUMBER_THREADS = \"$CPUCOUNT\"
	echo BB_GENERATE_MIRROR_TARBALLS = \"0\"

	echo DL_DIR = \"/opt/parabuild/etc/build/poky_dl_dir\" 
	echo CVSDIR = \"/opt/parabuild/etc/build/poky_cvs_dir\"
	echo GITDIR = \"/opt/parabuild/etc/build/poky_git_dir\"
	echo SVNDIR = \"/opt/parabuild/etc/build/poky_svn_dir\"

	#echo PARALLEL_MAKE = \"-j $CPUCOUNT\" 
) >> conf/local.conf

# Build firmware images, multiple machines can be built here
echo "*** Building ***"
if [ "x${SQUEEZEOS_DEBUG}" != "x" ]
then
	run_bitbake "squeezeos-image-debug"
else
	run_bitbake "squeezeos-image"
fi

# Do not leave the source code on the build machine, cleanup private modules
for PKG in 'marvell-wlan-tools-src' 'marvell-gspi-module-src' 'marvell-wps-src' 'squeezeplay-private'
do
	echo "*** Cleaning $PKG ***"
	run_bitbake "$PKG -c clean -f"
done

# QA: Check version numbers match
SQUEEZEOS_VERSION=`cat tmp-${MACHINE}/rootfs/etc/squeezeos.version | perl -nle 'print if s/^[\d\.]+\sr(\d+)$/$1/'`
SQUEEZEPLAY_VERSION=`strings tmp-${MACHINE}/rootfs/usr/bin/jive | perl -nle 'if(s/^(?:Squeezeplay )?[\d\.]+\sr(\d+)$/$1/){print;exit;}'`

echo "SQUEEZEOS_VERSION=${SQUEEZEOS_VERSION}"
echo "SQUEEZEPLAY_VERSION=${SQUEEZEPLAY_VERSION}"

if [ "x${SQUEEZEOS_VERSION}" != "x${SQUEEZEPLAY_VERSION}" ]
then
	echo "ERROR: squeezeos version mismatch"
fi
