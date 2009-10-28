require libdbd-sqlite-perl_${PV}.bb
inherit native

PR = "r6"

DEPENDS = "libdbi-perl sqlite3"
