DESCRIPTION="SoX is the Swiss Army knife of sound processing tools. \
It converts audio files among various standard audio file formats \
and can apply different effects and filters to the audio data." 
HOMEPAGE = "http://sox.sourceforge.net"
DEPENDS = "libogg libvorbis wavpack flac libmad libsamplerate"
SECTION = "audio"
LICENSE = "GPL"
PR = "r0"

SRC_URI = "${SOURCEFORGE_MIRROR}/sox/sox-${PV}.tar.gz"

CFLAGS_prepend = "-I${STAGING_INCDIR} "
export LDFLAGS = "-L${STAGING_LIBDIR} -static"

EXTRA_OECONF = "--with-flac \
		--with-vorbis \
		--with-ogg \
		--with-mad \
		--with-wavpack \
		--with-samplerate \
		--without-id3tag \
		--without-lame \
		--without-ffmpeg \
		--without-png \
		--without-ladspa \
		--disable-oss \
		--disable-alsa \
		--disable-symlinks \
		--disable-libao \
		--disable-coreaudio \
		--without-libltdl \
		--disable-shared"

inherit autotools

