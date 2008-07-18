DESCRIPTION = "LUA"
SECTION = "libs"
LICENSE = "MIT"

BV = "5.1.1"

PV = "${BV}+svnr${SRCREV}"

SRC_URI="${SQUEEZEPLAY_SCM};module=lua-${BV}"

S = "${WORKDIR}/lua-${BV}"

EXTRA_OEMAKE = "squeezeos"

do_stage() {
	oe_libinstall -C src liblua ${STAGING_LIBDIR}/
	install -m 0644 src/lua.h src/lualib.h src/lauxlib.h src/luaconf.h ${STAGING_INCDIR}/
	install -m 0755 src/lua src/luac ${STAGING_BINDIR}/
}

do_install() {
	mkdir ${D}/usr
	oe_runmake install INSTALL_TOP=${D}/usr
}

PACKAGES = "liblua5.1-dbg liblua5.1-dev liblua5.1 ${PN}-dbg ${PN}-doc ${PN}"

FILES_${PN} = "${bindir}"
FILES_${PN}-doc = "${mandir}"

FILES_liblua5.1 = "${libdir}/liblua.so.*"
FILES_liblua5.1-dev = "${libdir}/liblua.* ${includedir}"
