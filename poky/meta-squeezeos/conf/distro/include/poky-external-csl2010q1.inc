#
# Poky configuration to use external CSL 2010q1 toolchain (ARM EABI)
#

TARGET_VENDOR = "-none"

PREFERRED_PROVIDER_linux-libc-headers = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/arm-none-linux-gnueabi-gcc = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/arm-none-linux-gnueabi-gcc-initial = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/arm-none-linux-gnueabi-gcc-intermediate = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/arm-none-linux-gnueabi-binutils = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/arm-none-linux-gnueabi-libc-for-gcc = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/libc = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/libintl = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/libiconv = "external-csl-toolchain"
PREFERRED_PROVIDER_glibc-thread-db = "external-csl-toolchain"
PREFERRED_PROVIDER_virtual/linux-libc-headers = "external-csl-toolchain"

PREFERRED_VERSION_external-csl-toolchain = "2010q1-202"

TOOLCHAIN_OPTIONS = " --sysroot=${STAGING_DIR_HOST}"
