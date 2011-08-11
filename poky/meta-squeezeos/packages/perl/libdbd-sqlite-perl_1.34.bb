DESCRIPTION = "DBD::SQLite - SQLite driver for the Perl5 Database Interface (DBI)"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r18"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

ARM_INSTRUCTION_SET = "arm"

DEPENDS = "libdbi-perl libdbd-sqlite-perl-native"
DEPENDS = "libdbi-perl libdbi-perl-native sqlite3"

SRC_URI = "http://svn.slimdevices.com/repos/slim/7.6/trunk/vendor/CPAN/DBD-SQLite-${PV}_01.tar.gz"

S = "${WORKDIR}/DBD-SQLite-${PV}_01"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"

cpan_do_install() {
        # from cpan class
        if [ yes = "yes" ]; then
                oe_runmake install_vendor
        fi

        # Remove unnecessary large source files
        rm -rf ${D}/${prefix}/lib/perl5/auto/share/dist/DBD-SQLite
}


