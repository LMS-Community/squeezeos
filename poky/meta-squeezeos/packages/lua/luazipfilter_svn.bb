DESCRIPTION = "LUA zipfilter"
SECTION = "libs"
LICENSE = "copyright"

PV = "0.1+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua"

SRC_URI="${SQUEEZEPLAY_SCM};module=luazipfilter"

S = "${WORKDIR}/luazipfilter"

inherit autotools

do_install() {
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 .libs/zipfilter.so ${D}${libdir}/lua/5.1
}

PACKAGES = "liblua5.1-zipfilter-dbg liblua5.1-zipfilter"

FILES_liblua5.1-zipfilter-dbg = "${libdir}/lua/5.1/.debug"
FILES_liblua5.1-zipfilter = "${libdir}"
