SECTION = "libs"
PRIORITY = "optional"
DEPENDS = "zlib"
DESCRIPTION = "Library for interacting with ID3 tags + patches from Audio::Scan"
LICENSE = "GPL"

PV = "svnr${SRCREV}"
PR = "r14"

SRC_URI = "svn://svn.slimdevices.com/repos/opensource/tags/Audio-Scan/0.42;proto=http;module=libid3tag \
	file://id3tag.pc"

S = "${WORKDIR}/libid3tag"

inherit autotools pkgconfig

EXTRA_OECONF = "-enable-speed"

do_configure_prepend() {
    install -m 0644 ${WORKDIR}/id3tag.pc ${S}
}

do_stage() {
	oe_libinstall -so libid3tag ${STAGING_LIBDIR}
        install -m 0644 id3tag.h ${STAGING_INCDIR}
}
