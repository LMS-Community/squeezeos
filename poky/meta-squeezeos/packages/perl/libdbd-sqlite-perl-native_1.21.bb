require libdbd-sqlite-perl_${PV}.bb
inherit native

PR = "r2"

DEPENDS = "libdbi-perl sqlite3"
