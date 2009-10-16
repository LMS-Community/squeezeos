DESCRIPTION = "Audio::Scan - Fast C scanning of audio file metadata"
SECTION = "libs"
LICENSE = "GPL"
PV = "svnr${SRCREV}"
PR = "r5"
DEPENDS = "libid3tag"

SRC_URI = "svn://svn.slimdevices.com/repos/opensource/trunk;proto=http;module=Audio-Scan"

S = "${WORKDIR}/Audio-Scan"

inherit cpan

export INCLUDE = ${STAGING_LIBDIR}/../include
export ID3TAG_LIBS = ${STAGING_LIBDIR}
export NO_LIBID3TAG = 1

FILES_${PN}-dbg = "${PERLLIBDIRS}/auto/Audio/Scan/.debug"
FILES_${PN} = "${PERLLIBDIRS}"
