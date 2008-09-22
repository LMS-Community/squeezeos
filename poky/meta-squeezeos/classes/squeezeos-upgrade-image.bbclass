
# package root filesystem and kernel for SqueezeOS deployment

KERNEL_IMAGE_NAME ?= "zImage-${MACHINE}.bin"
ROOTFS_IMAGE_NAME ?= "${IMAGE_NAME}.rootfs.cramfs"

# export IMAGE_SQUEEZEOS_UPGRADE
IMAGE_SQUEEZEOS_UPGRADE[export] = "1"

do_squeezeos_image() {
	if [ "x$IMAGE_SQUEEZEOS_UPGRADE" == "x" ]; then
		exit 0
	fi

	tmpdir=`mktemp -d /tmp/squeezeos-XXXXXX`

	# Copy image files
	cp ${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGE_NAME} ${tmpdir}/zImage-P7
	cp ${DEPLOY_DIR_IMAGE}/${ROOTFS_IMAGE_NAME} ${tmpdir}/root.cramfs
	cp ${TMPDIR}/rootfs/etc/squeezeos.version ${tmpdir}/jive.version

	# Prepare files
	cd ${tmpdir}
	md5sum zImage-P7 root.cramfs  > upgrade.md5

	VERSION=${DISTRO_VERSION}_r${SQUEEZEOS_REVISION}

	# Create zip
	zip ${DEPLOY_DIR_IMAGE}/${MACHINE}_${VERSION}.bin *
	cd ${DEPLOY_DIR_IMAGE}

	rm -f ${MACHINE}.bin
	ln -s ${MACHINE}_${VERSION}.bin ${MACHINE}.bin

	# Cleanup
	rm -rf ${tmpdir}
}

addtask squeezeos_image after do_rootfs before do_build

squeezeos_version() {
	echo "${DISTRO_VERSION} r${SQUEEZEOS_REVISION}" > ${IMAGE_ROOTFS}/etc/squeezeos.version
	echo `whoami`@`hostname` `date` >> ${IMAGE_ROOTFS}/etc/squeezeos.version
}
