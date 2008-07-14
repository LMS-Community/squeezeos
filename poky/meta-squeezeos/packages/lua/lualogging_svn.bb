DESCRIPTION = "LUA logging"
SECTION = "libs"
LICENSE = "kelper"

BV = "1.1.2"

PV = "${BV}+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua"

SRC_URI="${SQUEEZEPLAY_SCM};module=lualogging-${BV}"

S = "${WORKDIR}/lualogging-${BV}"

do_install() {
	oe_runmake install LUA_DIR=${D}${datadir}/lua/5.1
}

PACKAGES = "liblua5.1-logging"

FILES_liblua5.1-logging = "${libdir} ${datadir}"
