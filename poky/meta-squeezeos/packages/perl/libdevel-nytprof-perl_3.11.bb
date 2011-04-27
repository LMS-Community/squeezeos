DESCRIPTION = "Devel::NYTProf - Powerful feature-rich perl source code profiler"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r15"
DEPENDS = "zlib"

# We want Perl to be as fast as possible, reset the default optimization flags
FULL_OPTIMIZATION = "-fexpensive-optimizations -fomit-frame-pointer -frename-registers -O2 -ggdb -feliminate-unused-debug-types"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/T/TI/TIMB/Devel-NYTProf-${PV}.tar.gz \
	file://libdevel-nytprof-perl-Makefile.PL.patch;patch=1"

S = "${WORKDIR}/Devel-NYTProf-${PV}"

inherit cpan

export INCLUDE = ${STAGING_LIBDIR}/../include

FILES_${PN} = "${PERLLIBDIRS}/auto/Devel/NYTProf/NYTProf.so \
               ${PERLLIBDIRS}/Devel/NYTProf.pm \
               ${PERLLIBDIRS}/Devel/NYTProf/*.pm"
