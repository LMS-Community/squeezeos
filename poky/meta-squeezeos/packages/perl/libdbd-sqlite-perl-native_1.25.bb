require libdbd-sqlite-perl_${PV}.bb
inherit native

PR = "r4"

DEPENDS = "libdbi-perl sqlite3"
