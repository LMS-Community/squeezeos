require lua.inc

inherit native

PR="r3"

do_stage() {
	install -m 0755 src/lua src/luac ${STAGING_BINDIR}/
}
