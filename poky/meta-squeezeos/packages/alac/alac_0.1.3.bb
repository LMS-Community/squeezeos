DESCRIPTION = "Simple Apple Lossless Decoder"
SECTION = "libs"
LICENSE = "LGPL"

PR="r0"

SRC_URI = "svn://svn.slimdevices.com/repos/slim/vendor/src;module=alac_decoder;rev=25280;proto=http"

S="${WORKDIR}/alac_decoder"


do_make() { 
	oe_runmake
} 

do_install() { 
	cp ${S}/alac ${D}/
}
	
FILES_${PN} = "alac"
