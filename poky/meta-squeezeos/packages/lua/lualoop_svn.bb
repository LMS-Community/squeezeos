DESCRIPTION = "LUA loop"
SECTION = "libs"
LICENSE = "MIT"

BV = "2.2-alpha"

PV = "${BV}+svnr${SRCREV}"
#PR="r0"

SRC_URI="${SQUEEZEPLAY_SCM};module=loop-${BV}"

S = "${WORKDIR}/loop-${BV}"

do_install() {
	mkdir -p ${D}/${datadir}/lua/5.1/loop
	mkdir ${D}/${datadir}/lua/5.1/loop/collection
	mkdir ${D}/${datadir}/lua/5.1/loop/debug
	install -m 0644 loop/base.lua ${D}/${datadir}/lua/5.1/loop
	install -m 0644 loop/simple.lua ${D}/${datadir}/lua/5.1/loop
	install -m 0644 loop/table.lua ${D}/${datadir}/lua/5.1/loop
	install -m 0644 loop/collection/ObjectCache.lua ${D}/${datadir}/lua/5.1/loop/collection
	install -m 0644 loop/debug/Viewer.lua ${D}/${datadir}/lua/5.1/loop/debug
}

PACKAGES = "liblua5.1-loop"

FILES_liblua5.1-loop = "${datadir}"
