DESCRIPTION = "EV - perl interface to libev, a high performance full-featured event loop"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r4"

SRC_URI = "http://backpan.perl.org/authors/id/M/ML/MLEHMANN/EV-${PV}.tar.gz"

S = "${WORKDIR}/EV-${PV}"

inherit cpan

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
