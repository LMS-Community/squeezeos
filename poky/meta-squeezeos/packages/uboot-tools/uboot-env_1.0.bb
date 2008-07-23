DESCRIPTION = "uboot fw-env tools"
SECTION = "base"
LICENSE = "GPL"

DEPENDS = "mtd-utils"

PR = "r2"

SRC_URI=" \
	file://Makefile \
	file://fw_env.c \
	file://fw_env.config \
	file://fw_env.h \
	file://fw_env_main.c \
	file://crc32.c \
	"

S = ${WORKDIR}

do_install() {
	install -m 0755 -d ${D}${sbindir}
	install -m 0755 ${WORKDIR}/fw_printenv ${D}${sbindir}/fw_printenv
	ln -s fw_printenv ${D}${sbindir}/fw_setenv
}
