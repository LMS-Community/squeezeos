DESCRIPTION = "baby msp430 firmware"
LICENSE = "Confidential"

PV = "svnr${SRCREV}"
PR="r1"

PROVIDES = "baby-msp430"

SRC_URI="${SQUEEZEOS_PRIVATE_SVN};module=baby_msp430"

S = "${WORKDIR}/baby_msp430/src"

do_compile() {
	oe_runmake all-txt
}

do_install() {
	INSTALL_DIR=${D}${base_libdir}/firmware

	mkdir -p ${INSTALL_DIR}
	install -m 0644 ${S}/msp430-0001.txt ${INSTALL_DIR}/msp430-0001.txt
	install -m 0644 ${S}/msp430-0002.txt ${INSTALL_DIR}/msp430-0002.txt
	install -m 0644 ${S}/msp430-0003.txt ${INSTALL_DIR}/msp430-0003.txt
}

PACKAGES = "baby-msp430"

FILES_baby-msp430 = "${base_libdir}/firmware"
