DESCRIPTION = "Tools for managing memory technology devices."
SECTION = "base"
DEPENDS = "zlib lzo"
HOMEPAGE = "http://www.linux-mtd.infradead.org/"
LICENSE = "GPLv2"
PR = "r2"

SRC_URI = "git://git.infradead.org/mtd-utils.git;protocol=git;tag=487550498f70455f083cdc82b65442596fe7308e \
	   file://new-ubi-git.patch;patch=1 \
	   file://nanddump_skip_bad_blocks.patch;patch=1 \
	   file://nandwrite_input_stdio.patch;patch=1"

S = "${WORKDIR}/git/"

EXTRA_OEMAKE = "'CC=${CC}' 'CFLAGS=${CFLAGS} -I${S}/include -Iinclude -Isrc -DWITHOUT_XATTR' \
	'RAWTARGETS=flash_eraseall nanddump nandwrite nandtest'"

do_stage () {
	install -d ${STAGING_INCDIR}/mtd
	for f in ${S}/include/mtd/*.h; do
		install -m 0644 $f ${STAGING_INCDIR}/mtd/
	done
}

do_install () {
	oe_runmake install DESTDIR=${D}
}

PARALLEL_MAKE = ""


PACKAGES = "ubi-utils-dbg ${PN}-dbg ubi-utils ${PN} ${PN}-doc"

FILES_ubi-utils = "${sbindir}/ubi*"
FILES_ubi-utils-dbg = "${sbindir}/.debug/ubi*"
