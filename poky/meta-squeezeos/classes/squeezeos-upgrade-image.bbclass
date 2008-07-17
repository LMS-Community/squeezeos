
# package root filesystem and kernel for SqueezeOS deployment

KERNEL_IMAGE_NAME ?= "zImage-${MACHINE}.bin"
ROOTFS_IMAGE_NAME ?= "${IMAGE_NAME}.rootfs.cramfs"

do_squeezeos_image() {
	tmpdir=`mktemp -d /tmp/squeezeos-XXXXXX`

	# Copy image files
	cp ${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGE_NAME} ${tmpdir}/zImage-P7
	cp ${DEPLOY_DIR_IMAGE}/${ROOTFS_IMAGE_NAME} ${tmpdir}/root.cramfs

	# Prepare files
	cd ${tmpdir}
	md5sum zImage-P7 root.cramfs  > upgrade.md5

	# FIXME version
	VERSION=${DISTRO_VERSION}_${SQUEEZEOS_REVISION}

	# Create zip
	zip ${DEPLOY_DIR_IMAGE}/jive_${VERSION}.bin *
	cd ${DEPLOY_DIR_IMAGE}

	rm jive.bin
	ln -s jive_${VERSION}.bin jive.bin

	# Cleanup
	rm -rf ${tmpdir}
}

addtask squeezeos_image after do_rootfs before do_build

squeezeos_version() {
	echo "${DISTRO_VERSION} ${SQUEEZEOS_REVISION}" > ${IMAGE_ROOTFS}/etc/squeezeos.version
	echo `whoami`@`hostname` `date` >> ${IMAGE_ROOTFS}/etc/squeezeos.version
}
