# This file builds the squeezeplay specific rtmp.so library which is loaded by Rtmp.lua to support rtmp playback
# It depends on include files which are only staged once squeezeplay is built and is therefore not built as part of squeezeplay

DESCRIPTION = "LUA rtmp"
SECTION = "libs"
LICENSE = "copyright"

PV = "0.1+svnr${SRCREV}"
PR="r1"

DEPENDS = "lua squeezeplay"

SRC_URI="${SQUEEZEPLAY_SCM};module=luartmp-squeezeplay"

S = "${WORKDIR}/luartmp-squeezeplay"

do_compile() {
	${CC} -I${STAGING_INCDIR}/squeezeplay -I${STAGING_INCDIR}/squeezeplay/ui -I${STAGING_INCDIR}/lua -I${STAGING_INCDIR}/SDL -shared rtmp.c -o rtmp.so
}

do_install() {
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 rtmp.so ${D}${libdir}/lua/5.1
}

PACKAGES = "liblua5.1-luartmp-dbg liblua5.1-luartmp"

FILES_liblua5.1-luartmp-dbg = "${libdir}/lua/5.1/.debug"
FILES_liblua5.1-luartmp = "${libdir}"
