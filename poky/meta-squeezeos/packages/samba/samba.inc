SECTION = "console/network"
LICENSE = "GPL"
DEPENDS = "readline virtual/libiconv"

SRC_URI = "http://samba.org/samba/ftp/stable/samba-${PV}.tar.gz \
           file://configure.patch;patch=1 \
           file://makefilein.patch;patch=1 \
           file://cifs.patch;patch=1"

S = "${WORKDIR}/samba-${PV}/source"

inherit autotools

EXTRA_OECONF = " \
	--disable-cups --with-readline=${STAGING_LIBDIR}/.. \
	--with-libiconv=${STAGING_LIBDIR}/.. \
	--without-ads --without-automount --without-smbmount --without-syslog \
	--without-ldapsam --without-quotas --without-nisplus-home \
	--without-sys-quotas --without-libsmbclient --without-winbind \
	--without-acl-support \
	"

do_configure_prepend () {
	./script/mkversion.sh
	if [ ! -e acinclude.m4 ]; then
		cat aclocal.m4 > acinclude.m4
	fi
}

do_compile () {

## Use to link statically
##	oe_runmake proto_exists all
##	base_do_compile
##	${CC} client/mount.cifs.c -o mount.cifs

## Use to link with shared library
	oe_runmake proto_exists shared
	base_do_compile
}

