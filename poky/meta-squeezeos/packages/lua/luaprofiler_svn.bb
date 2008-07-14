DESCRIPTION = "LUA profiler"
SECTION = "libs"
LICENSE = "kelper"

BV = "2.0"

PV = "${BV}+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua"

SRC_URI="${SQUEEZEPLAY_SCM};module=luaprofiler-${BV}"

S = "${WORKDIR}/luaprofiler-${BV}"

do_compile() {
	oe_runmake -f Makefile.linux
}

do_install() {
	mkdir -p ${D}/${libdir}/lua/5.1
	oe_runmake -f Makefile.linux install PREFIX=${D}/usr
}

PACKAGES = "liblua5.1-profiler-dbg liblua5.1-profiler"

FILES_liblua5.1-profiler-dbg = "${libdir}/lua/5.1/.debug"
FILES_liblua5.1-profiler = "${libdir} ${datadir}"
