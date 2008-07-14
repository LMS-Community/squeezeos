DESCRIPTION = "LUA json"
SECTION = "libs"
LICENSE = "copyright"

PV = "0.1+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua"

SRC_URI="${SQUEEZEPLAY_SCM};module=luajson"

S = "${WORKDIR}/luajson"

inherit autotools

do_install() {
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 .libs/json.so ${D}${libdir}/lua/5.1
}

PACKAGES = "liblua5.1-json-dbg liblua5.1-json"

FILES_liblua5.1-json-dbg = "${libdir}/lua/5.1/.debug"
FILES_liblua5.1-json = "${libdir}"
