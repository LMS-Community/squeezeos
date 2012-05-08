DESCRIPTION = "baby msp430 firmware"
LICENSE = "Confidential"

PR="r45"

PROVIDES = "baby-msp430"

SRC_URI=" \
	file://msp430-0001.txt \
	file://msp430-0003.txt \
	"

do_install() {
	INSTALL_DIR=${D}${base_libdir}/firmware

	mkdir -p ${INSTALL_DIR}
	install -m 0644 ${WORKDIR}/msp430-0001.txt ${INSTALL_DIR}/msp430-0001.txt
	install -m 0644 ${WORKDIR}/msp430-0001.txt ${INSTALL_DIR}/msp430-0002.txt
	install -m 0644 ${WORKDIR}/msp430-0003.txt ${INSTALL_DIR}/msp430-0003.txt
	install -m 0644 ${WORKDIR}/msp430-0003.txt ${INSTALL_DIR}/msp430-0004.txt
	install -m 0644 ${WORKDIR}/msp430-0003.txt ${INSTALL_DIR}/msp430-0005.txt
	install -m 0644 ${WORKDIR}/msp430-0003.txt ${INSTALL_DIR}/msp430-0006.txt
	install -m 0644 ${WORKDIR}/msp430-0003.txt ${INSTALL_DIR}/msp430-0007.txt
}

PACKAGES = "baby-msp430"

FILES_baby-msp430 = "${base_libdir}/firmware"
