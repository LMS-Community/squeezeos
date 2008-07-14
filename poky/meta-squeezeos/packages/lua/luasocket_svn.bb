DESCRIPTION = "LUA socket"
SECTION = "libs"
LICENSE = "MIT"

BV = "2.0.1"

PV = "${BV}+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua"

SRC_URI="${SQUEEZEPLAY_SCM};module=luasocket-${BV}"

S = "${WORKDIR}/luasocket-${BV}"

EXTRA_OEMAKE = "PLATFORM=linux LDFLAGS="

do_install() {
	mkdir ${D}/usr
	oe_runmake install INSTALL_TOP=${D}/usr
}

PACKAGES = "liblua5.1-socket-dbg liblua5.1-socket"

FILES_liblua5.1-socket-dbg = "${libdir}/lua/5.1/*/.debug"
FILES_liblua5.1-socket = "${libdir} ${datadir}"
