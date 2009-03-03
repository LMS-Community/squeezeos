DESCRIPTION="SoX is the Swiss Army knife of sound processing tools. \
It converts audio files among various standard audio file formats \
and can apply different effects and filters to the audio data." 
HOMEPAGE = "http://sox.sourceforge.net"
SECTION = "audio"
LICENSE = "GPL"
PR = "r0"

SRC_URI = "${SOURCEFORGE_MIRROR}/sox/sox-${PV}.tar.gz"

S = "${WORKDIR}/sox-${PV}"

inherit autotools


