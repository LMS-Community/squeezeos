DESCRIPTION = "EV - perl interface to libev, a high performance full-featured event loop"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r0"

SRC_URI = "http://backpan.perl.org/authors/id/M/ML/MLEHMANN/EV-${PV}.tar.gz"

S = "${WORKDIR}/EV-${PV}"

inherit cpan

do_configure() {
		# Don't want signalfd because we do not have a capable kernel (2.6.27 or later)
		# and the test in Makefile.PL will produce a false positive
		export EV_SIGNALFD=0

		cpan_do_configure
}

cpan_do_install() {
        # from cpan class
        if [ yes = "yes" ]; then
                oe_runmake install_vendor
        fi

        # Remove unnecessary large headers
        rm -r ${D}/${prefix}/lib/perl5/EV/*.h
}

FILES_${PN} = "${PERLLIBDIRS}/auto/EV/* \
               ${PERLLIBDIRS}"
