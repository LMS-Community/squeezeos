DESCRIPTION = "DBI - Database indpendent interface for Perl"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r8"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

ARM_INSTRUCTION_SET = "arm"

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
