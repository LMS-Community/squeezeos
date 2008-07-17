DESCRIPTION="cramfs"
SECTION="base"
LICENSE="GPL"
PR="r1"

DEPENDS = "zlib-native"

SRC_URI = "\
	${SOURCEFORGE_MIRROR}/cramfs/cramfs-${PV}.tar.gz \
	file://cramfs_fix_u32.patch;patch=1 \
	"

S = "${WORKDIR}/cramfs-${PV}"

inherit native

do_compile() {
	oe_runmake all
}

do_stage() {
	install -m 755 mkcramfs ${STAGING_BINDIR}
}
