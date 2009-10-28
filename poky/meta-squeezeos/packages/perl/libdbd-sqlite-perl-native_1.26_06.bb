require libdbd-sqlite-perl_${PV}.bb
inherit native

PR = "r5"

DEPENDS = "libdbi-perl sqlite3"
