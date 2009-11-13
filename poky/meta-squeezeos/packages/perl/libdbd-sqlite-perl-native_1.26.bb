require libdbd-sqlite-perl_${PV}.bb
inherit native

PR = "r7"

DEPENDS = "libdbi-perl sqlite3"
