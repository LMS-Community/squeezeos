DESCRIPTION = "LUA filesystem"
SECTION = "libs"
LICENSE = "Kepler"

BV = "1.2"

PV = "${BV}+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua"

SRC_URI="${SQUEEZEPLAY_SCM};module=luafilesystem-${BV}"

S = "${WORKDIR}/luafilesystem-${BV}"

EXTRA_OEMAKE = "PLATFORM=linux"

do_install() {
	oe_runmake install LUA_LIBDIR=${D}/${libdir}/lua/5.1
}

PACKAGES = "liblua5.1-filesystem-dbg liblua5.1-filesystem"

FILES_liblua5.1-filesystem-dbg = "${libdir}/lua/5.1/.debug"
FILES_liblua5.1-filesystem = "${libdir} ${datadir}"
