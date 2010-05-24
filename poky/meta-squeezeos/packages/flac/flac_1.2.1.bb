DESCRIPTION = "FLAC"
SECTION = "libs"
LICENSE = "Xiph"

PR = "r6"

SRC_URI="${SOURCEFORGE_MIRROR}/flac/flac-${PV}.tar.gz \
	 file://bitreader-1.2.1.patch;patch=1 \
	 file://compile-fix.patch;patch=1 \
         file://arm-asm.patch;patch=1"

S = "${WORKDIR}/flac-${PV}"

inherit autotools gettext

CFLAGS_prepend = "-DFLAC__CPU_ARM "
EXTRA_OECONF = "--disable-ogg --disable-xmms-plugin --enable-static"

do_stage() {
	autotools_stage_all
}

PACKAGES += "libflac libflac++ liboggflac liboggflac++"
FILES_${PN} = "${bindir}/*"
FILES_libflac = "${libdir}/libFLAC.so.*"
FILES_libflac++ = "${libdir}/libFLAC++.so.*"
FILES_liboggflac = "${libdir}/libOggFLAC.so.*"
FILES_liboggflac++ = "${libdir}/libOggFLAC++.so.*"
