DESCRIPTION = "DBD::SQLite - SQLite driver for the Perl5 Database Interface (DBI)"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r7"

DEPENDS = "libdbi-perl libdbd-sqlite-perl-native"
DEPENDS = "libdbi-perl libdbi-perl-native sqlite3"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/A/AD/ADAMK/DBD-SQLite-${PV}_06.tar.gz"

S = "${WORKDIR}/DBD-SQLite-${PV}_06"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
