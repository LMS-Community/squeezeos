DESCRIPTION = "XML::Parser::Expat - Low level access to James Clark's expat XML parser"
SECTION = "libs"
LICENSE = "Artistic|GPL"
PR = "r3"

DEPENDS = "expat"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/M/MS/MSERGEANT/XML-Parser-${PV}.tar.gz \
	file://libxml-parser-perl-Expat-Makefile.PL.patch;patch=1"

S = "${WORKDIR}/XML-Parser-${PV}"

EXTRA_CPANFLAGS = "EXPATLIBPATH=${STAGING_LIBDIR} EXPATINCPATH=${STAGING_INCDIR}"

inherit cpan

FILES_${PN}-doc = "${PERLLIBDIRS}/*.pod"
FILES_${PN} = "${PERLLIBDIRS}"
