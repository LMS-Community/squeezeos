DESCRIPTION = "Linux kernel for jive devices"
SECTION = "kernel"
LICENSE = "GPL"

#PR = "r0"

inherit kernel

SRC_URI = " \
          ${SQUEEZEOS_SVN}/s3c2412;proto=http;module=linux-${PV} \
          file://defconfig-jive \
          "

S = "${WORKDIR}/linux-${PV}"

COMPATIBLE_MACHINE = "(jive)"

do_configure_prepend() {

	rm -f ${S}/.config || true

        if [ "${TARGET_OS}" = "linux-gnueabi" -o  "${TARGET_OS}" = "linux-uclibcgnueabi" ]; then
                echo "CONFIG_AEABI=y"                   >> ${S}/.config
                echo "CONFIG_OABI_COMPAT=y"             >> ${S}/.config
        else
                echo "# CONFIG_AEABI is not set"        >> ${S}/.config
                echo "# CONFIG_OABI_COMPAT is not set"  >> ${S}/.config
        fi

        sed     -e '/CONFIG_AEABI/d' \
                -e '/CONFIG_OABI_COMPAT=/d' \
                '${WORKDIR}/defconfig-${MACHINE}' >>'${S}/.config'

        yes '' | oe_runmake oldconfig

}

do_deploy() {
	install -d ${DEPLOY_DIR_IMAGE}
	install -m 0644 arch/${ARCH}/boot/${KERNEL_IMAGETYPE} ${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGETYPE}-${PV}-${MACHINE}-${DATETIME}.bin
	cd ${DEPLOY_DIR_IMAGE}
	ln -sf ${KERNEL_IMAGETYPE}-${PV}-${MACHINE}-${DATETIME}.bin ${KERNEL_IMAGETYPE}-${MACHINE}.bin
	tar -cvzf ${DEPLOY_DIR_IMAGE}/modules-${KERNEL_RELEASE}-${MACHINE}.tgz -C ${D} lib	
}
