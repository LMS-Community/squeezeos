#!/bin/sh

echo "Setting environment variables..."
source poky-init-build-env

echo "Cleaning out squeezeos-image and squeezeplay"
bitbake squeezeos-image -c clean
bitbake squeezeplay -c clean

echo "Removing old binaries"
rm -rf $PARABUILD_BUILD_DIR/build/tmp/deploy/images/*.bin
rm -rf $PARABUILD_BUILD_DIR/build/tmp/deploy/images/squeezeos-image-*
