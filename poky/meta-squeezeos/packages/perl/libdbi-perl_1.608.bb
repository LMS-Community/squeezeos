DESCRIPTION = "DBI - Database indpendent interface for Perl"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r4"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/T/TI/TIMB/DBI-${PV}.tar.gz"

S = "${WORKDIR}/DBI-${PV}"

inherit cpan

cpan_do_install() {
	# from cpan class
	if [ yes = "yes" ]; then
		oe_runmake install_vendor
	fi

	# Remove unnecessary large headers
	rm -rf ${D}/${prefix}/lib/perl5/auto/DBI/*.h
	rm -rf ${D}/${prefix}/lib/perl5/auto/DBI/Driver.xst
}

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
