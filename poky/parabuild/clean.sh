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

TMPDIR="${PARABUILD_BUILD_DIR}/build/tmp-${MACHINE}"

if [ -d ${TMPDIR}/work ]
then
	echo "Cleaning out squeezeos-image and squeezeplay"
	bitbake squeezeos-image -c clean
	bitbake squeezeplay -c clean
fi

echo "Removing old binaries"
rm -rf ${TMPDIR}/deploy/images/${MACHINE}*bin
rm -rf ${TMPDIR}/deploy/images/squeezeos-image-${MACHINE}*
