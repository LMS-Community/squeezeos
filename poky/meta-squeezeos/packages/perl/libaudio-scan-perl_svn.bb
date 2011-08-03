DESCRIPTION = "Audio::Scan - Fast C scanning of audio file metadata"
SECTION = "libs"
LICENSE = "GPL"
PV = "svnr${SRCREV}"
PR = "r27"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

ARM_INSTRUCTION_SET = "arm"

# This should match the version currently listed in SBS's modules.conf file
SRC_URI = "svn://svn.slimdevices.com/repos/opensource/tags/Audio-Scan;proto=http;module=0.90"

S = "${WORKDIR}/0.90"

inherit cpan

export INCLUDE = ${STAGING_LIBDIR}/../include

FILES_${PN}-dbg = "${PERLLIBDIRS}/auto/Audio/Scan/.debug"
FILES_${PN} = "${PERLLIBDIRS}"
