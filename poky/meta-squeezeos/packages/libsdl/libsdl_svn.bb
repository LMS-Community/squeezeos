DESCRIPTION = "SDL"
SECTION = "libs"
LICENSE = "LGPL"

BV = "1.2.13"

PV = "${BV}+svnr${SRCREV}"
PR = "r3"

DEPENDS = "alsa-lib"
RDEPENDS = "alsa-lib"

# SDL has the hottest functions in the system, and Thumb is much slower for these
ARM_INSTRUCTION_SET = "arm"

SRC_URI = " \
	${SQUEEZEPLAY_SCM};module=SDL-${BV} \
	file://fbvideo-no-timings.patch;patch=1 \
	"

S = "${WORKDIR}/SDL-${BV}"

inherit autotools binconfig

PARALLEL_MAKE = ""

EXTRA_OECONF = "--enable-audio --enable-video --enable-events --disable-joystick --disable-cdrom --enable-threads -enable-timers --enable-file --enable-loadso --disable-oss --disable-alsa --disable-esd --disable-arts --disable-video-x11 --disable-video-directfb --enable-clock_gettime"

autotools_do_configure() {
	${S}/autogen.sh
	oe_runconf
}

do_stage() {
	autotools_stage_all
	rm ${STAGING_LIBDIR}/libSDL.la
}

