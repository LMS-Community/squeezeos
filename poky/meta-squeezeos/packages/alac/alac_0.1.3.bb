DESCRIPTION = "Simple Apple Lossless Decoder"
SECTION = "libs"
LICENSE = "LGPL"

PR="r0"

SRC_URI = "http://crazney.net/programs/itunes/files/alac_decoder-${PV}.tar.gz \
           file://main.c.patch;patch=1"

S="${WORKDIR}/alac_decoder"


do_make() { 
	oe_runmake
} 

do_install() { 
	cp ${S}/alac ${D}/
}
	
FILES_${PN} = "alac"
