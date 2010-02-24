require samba.inc

PR = "r11"

SRC_URI += "file://config-lfs.patch;patch=1 \
	   file://smb.conf.dist \
	   file://smbusers \
	   file://status \
	   file://smbpasswd \
	   file://samba \
           file://quota.patch;patch=1;pnum=0 \
	   "

# The file system settings --foodir=dirfoo and overridden unconditionally
# in the samba config by --with-foodir=dirfoo - even if the --with is not
# specified!  Fix that here.  Set the privatedir to /etc/samba/private.
EXTRA_OECONF += "\
	samba_cv_struct_timespec=yes \
	--with-configdir=${sysconfdir}/samba \
	--with-privatedir=${sysconfdir}/samba/private \
	--with-lockdir=${localstatedir}/lock \
	--with-piddir=${localstatedir}/run \
	--with-logfilebase=${localstatedir}/log \
	"

PARALLEL_MAKE = ""

do_install() {
	# Install configuration file
	install -m 0755 -d "${D}${sysconfdir}/samba"
	install -m 0644 ${WORKDIR}/smb.conf.dist ${D}${sysconfdir}/samba/smb.conf.dist

	# Install user mapping file (contains root = "Squeezebox")
	install -m 0644 ${WORKDIR}/smbusers ${D}${sysconfdir}/samba/smbusers

	# Install the enabled/disabled status file. 
	install -m 0644 ${WORKDIR}/status ${D}${sysconfdir}/samba/status

	# Install minimal smb password file (contains root - 1234)
	install -m 0755 -d "${D}${sysconfdir}/samba/private"
	install -m 0644 ${WORKDIR}/smbpasswd ${D}${sysconfdir}/samba/private/smbpasswd

	# Install init script
	install -m 0755 -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/samba ${D}${sysconfdir}/init.d/samba

## Use to link statically
##	# Install samba and name server
##	install -m 0755 -d ${D}${sbindir}
##	install -m 0755 ${S}/bin/smbd ${D}${sbindir}/smbd
##	install -m 0755 ${S}/bin/nmbd ${D}${sbindir}/nmbd
##	# Install samba user binary
##	install -m 0755 -d ${D}${bindir}
##	install -m 0755 ${S}/bin/smbpasswd ${D}${bindir}/smbpasswd

## Use to link with shared library
	# Install samba shared libary
	install -m 0755 -d ${D}${libdir}
	install -m 0755 ${S}/bin/libsmb.so ${D}${libdir}/libsmb.so
	# Install samba and name server
	install -m 0755 -d ${D}${sbindir}
	install -m 0755 ${S}/bin/smbd.shared ${D}${sbindir}/smbd
	install -m 0755 ${S}/bin/nmbd.shared ${D}${sbindir}/nmbd
	# Install samba user binary
	install -m 0755 -d ${D}${bindir}
	install -m 0755 ${S}/bin/smbpasswd.shared ${D}${bindir}/smbpasswd
}

PACKAGES = "samba-dbg samba"

FILES_${PN} += "${libdir}"
FILES_${PN}-dbg += "${libdir}/.debug"

