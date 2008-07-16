DESCRIPTION="cramfs"
SECTION="base"
LICENSE="GPL"

DEPENDS = "zlib-native"

SRC_URI = "${SOURCEFORGE_MIRROR}/cramfs/cramfs-${PV}.tar.gz"

S = "${WORKDIR}/cramfs-${PV}"

inherit native

do_compile() {
	oe_runmake all
}

do_stage() {
	install -m 755 mkcramfs ${STAGING_BINDIR}
}
