require libdbd-sqlite-perl_${PV}.bb
inherit native

PR = "r9"

DEPENDS = "libdbi-perl sqlite3"
