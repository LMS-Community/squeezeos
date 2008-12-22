DESCRIPTION = "SDL Pango library"
SECTION = "libs"
LICENSE = "LGPL"

BV = "0.1.2"
	
PV = "${BV}"
PR = "r13"

DEPENDS = "glib-2.0 fontconfig freetype libsdl pango"
RDEPENDS = "libsdl pango"

SRC_URI = "${SOURCEFORGE_MIRROR}/sdlpango/SDL_Pango-${PV}.tar.gz \
	file://rebootstrap.patch;patch=1 \
	file://api_additions.patch;patch=1 \
	file://matrix_declarations.patch;patch=1 \
	file://fillrect_crash.patch;patch=1 \
	file://blit_overflow.patch;patch=1 \
	file://sp-performance1.patch;patch=1"

#SRC_URI="${SQUEEZEPLAY_SCM};module=SDL_Pango-${BV}"

S = "${WORKDIR}/SDL_Pango-${BV}"

inherit autotools

EXTRA_OECONF = "--enable-explicit-deps=no"

do_configure_append() {
	for i in $(find ${S} -name Makefile) ; do
		sed -i -e s:/usr/include/SDL:${STAGING_INCDIR}/SDL:g $i
		sed -i -e s:-L/usr/lib:-L${STAGING_LIBDIR}:g $i
	done
}

autotools_do_configure() {
	#aclocal
	#automake --foreign --include-deps --add-missing --copy
	#autoconf
	#${S}/autogen.sh
	oe_runconf
}

do_stage() {
	autotools_stage_all
}
