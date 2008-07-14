DESCRIPTION = "LUA syslog"
SECTION = "libs"
LICENSE = "public domain"

BV = "4"

PV = "${BV}+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua"

SRC_URI="${SQUEEZEPLAY_SCM};module=lsyslog-${BV}"

S = "${WORKDIR}/lsyslog-${BV}"

do_compile() {
	oe_runmake syslog.so
}

do_install() {
	mkdir -p ${D}/${libdir}/lua/5.1
	install -m 0755 syslog.so ${D}${libdir}/lua/5.1
}

PACKAGES = "liblua5.1-syslog-dbg liblua5.1-syslog"

FILES_liblua5.1-syslog-dbg = "${libdir}/lua/5.1/.debug"
FILES_liblua5.1-syslog = "${libdir}"
