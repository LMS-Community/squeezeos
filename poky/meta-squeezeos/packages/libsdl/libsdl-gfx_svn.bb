DESCRIPTION = "SDL image library"
SECTION = "libs"
LICENSE = "LGPL"

BV = "2.0.15"

PV = "${BV}+svnr${SRCREV}"
#PR = "r0"

# SDL has the hottest functions in the system, and Thumb is much slower for these
ARM_INSTRUCTION_SET = "arm"

SRC_URI="${SQUEEZEPLAY_SCM};module=SDL_gfx-${BV}"

S = "${WORKDIR}/SDL_gfx-${BV}"

DEPENDS = "libsdl freetype"
RDEPENDS = "libsdl"

inherit autotools

EXTRA_OECONF = "--disable-mmx"

do_stage() {
	autotools_stage_all
}
