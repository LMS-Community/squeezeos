require libdbd-sqlite-perl_${PV}.bb
inherit native

PR = "r8"

DEPENDS = "libdbi-perl sqlite3"
