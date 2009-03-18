DESCRIPTION = "LUA md5/sha"
SECTION = "libs"
LICENSE = "Public domain"

PV = "0.1+svnr${SRCREV}"
#PR="r0"

DEPENDS = "lua"

SRC_URI="${SQUEEZEPLAY_SCM};module=luamd5"

S = "${WORKDIR}/luamd5"

do_compile() {
	${MAKE} CC="${CC}" LUA=${STAGING_DIR}/${HOST_SYS}/usr MYNAME=sha1
        ${MAKE} CC="${CC}" LUA=${STAGING_DIR}/${HOST_SYS}/usr MYNAME=md5
}

do_install() {
	mkdir -p ${D}${libdir}/lua/5.1
	install -m 0755 md5.so ${D}${libdir}/lua/5.1
	install -m 0755 sha1.so ${D}${libdir}/lua/5.1
}

PACKAGES = "liblua5.1-md5-dbg liblua5.1-md5"

FILES_liblua5.1-md5-dbg = "${libdir}/lua/5.1/.debug"
FILES_liblua5.1-md5 = "${libdir}"
