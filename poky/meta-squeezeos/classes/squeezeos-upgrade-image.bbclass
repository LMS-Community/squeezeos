
# package root filesystem and kernel for SqueezeOS deployment

KERNEL_IMAGE_NAME ?= "${KERNEL_IMAGETYPE}-${MACHINE}.bin"
ROOTFS_IMAGE_NAME ?= "${IMAGE_NAME}.rootfs.cramfs"

# export IMAGE_SQUEEZEOS_UPGRADE
IMAGE_SQUEEZEOS_UPGRADE[export] = "1"

do_squeezeos_image() {
	if [ "x$IMAGE_SQUEEZEOS_UPGRADE" == "x" ]; then
		exit 0
	fi

	tmpdir=`mktemp -d /tmp/squeezeos-XXXXXX`

	# Copy image files
	# The kernel always has to be called zImage for compatibility with older
	# firwmare, even if it's uncompressed
	cp ${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGE_NAME} ${tmpdir}/zImage${IMAGE_SQUEEZEOS_EXTRA_VERSION}
	cp ${DEPLOY_DIR_IMAGE}/${ROOTFS_IMAGE_NAME} ${tmpdir}/root.cramfs
	cp ${TMPDIR}/rootfs/etc/squeezeos.version ${tmpdir}/jive.version
	echo -e ${IMAGE_SQUEEZEOS_BOARD_VERSION} > ${tmpdir}/board.version

	# Prepare files
	cd ${tmpdir}
	md5sum zImage${IMAGE_SQUEEZEOS_EXTRA_VERSION} root.cramfs  > upgrade.md5

	VERSION=${DISTRO_RELEASE}_r${SQUEEZEOS_REVISION}

	# Create zip
	rm -f ${DEPLOY_DIR_IMAGE}/${MACHINE}_${VERSION}.bin
	zip ${DEPLOY_DIR_IMAGE}/${MACHINE}_${VERSION}.bin jive.version board.version upgrade.md5 zImage${IMAGE_SQUEEZEOS_EXTRA_VERSION} root.cramfs
	cd ${DEPLOY_DIR_IMAGE}

	rm -f ${MACHINE}.bin
	ln -s ${MACHINE}_${VERSION}.bin ${MACHINE}.bin

	# Cleanup
	rm -rf ${tmpdir}
}

addtask squeezeos_image after do_rootfs before do_build

squeezeos_version() {
	echo "${DISTRO_RELEASE} r${SQUEEZEOS_REVISION}" > ${IMAGE_ROOTFS}/etc/squeezeos.version
	echo `whoami`@`hostname` `date` >> ${IMAGE_ROOTFS}/etc/squeezeos.version
}
