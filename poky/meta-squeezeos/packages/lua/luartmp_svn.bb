DESCRIPTION = "LUA rtmp"
SECTION = "libs"
LICENSE = "copyright"

PV = "0.1+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua squeezeplay"

SRC_URI="${SQUEEZEPLAY_SCM};module=luartmp-squeezeplay"

S = "${WORKDIR}/luartmp"

do_install() {
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 .libs/rtmp.so ${D}${libdir}/lua/5.1
}

PACKAGES = "liblua5.1-luartmp-dbg liblua5.1-luartmp"

FILES_liblua5.1-luartmp-dbg = "${libdir}/lua/5.1/.debug"
FILES_liblua5.1-luartmp = "${libdir}"
