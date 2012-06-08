#!/bin/sh

echo "Setting environment variables..."
source poky-init-build-env

# Check if a machine type exists
if [ "x${MACHINE}" != "x" ]
then
        echo "*** Building for MACHINE type: ${MACHINE}"
else
        echo "ERROR: No MACHINE specified!"
        exit 1
fi

TMPDIR="tmp-${MACHINE}"

# [AWY] I think maybe it is necessary to run the bitbake clean tasks
# below even when we do not yet have a TMPDIR because it sets up
# some pre-requisite state.
mkdir -p ${TMPDIR}/work

if [ -d ${TMPDIR}/work ]
then
	echo "Cleaning out squeezeos-image and squeezeplay"
	bitbake squeezeos-image -c clean
	bitbake squeezeplay -c clean
fi

echo "Removing old binaries"
rm -rf ${TMPDIR}/deploy/images/${MACHINE}*bin
rm -rf ${TMPDIR}/deploy/images/squeezeos-image-${MACHINE}*
