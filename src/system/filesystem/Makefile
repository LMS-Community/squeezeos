ARCH=arm-9tdmi-linux-gnu
LIB_DIR=../../../toolchain/gcc-4.1.1-glibc-2.3.2/$(ARCH)/$(ARCH)/lib

install:
	mkdir -p ${PREFIX} ${PREFIX}/dev ${PREFIX}/etc ${PREFIX}/etc/init.d ${PREFIX}/bin ${PREFIX}/lib ${PREFIX}/proc ${PREFIX}/mnt ${PREFIX}/mnt/yaffs ${PREFIX}/mnt/overlay ${PREFIX}/mnt/mmc ${PREFIX}/tmp ${PREFIX}/var ${PREFIX}/var/shm ${PREFIX}/var/log ${PREFIX}/var/run ${PREFIX}/etc/network ${PREFIX}/etc/dropbear ${PREFIX}/usr/lib ${PREFIX}/usr/bin ${PREFIX}/root
	chmod 755 . ${PREFIX}/dev ${PREFIX}/etc ${PREFIX}/etc/init.d ${PREFIX}/bin ${PREFIX}/proc ${PREFIX}/mnt ${PREFIX}/tmp ${PREFIX}/var ${PREFIX}/var/shm
	mkdir -p ${PREFIX}/etc/dropbear
	install linuxrc ${PREFIX}
	install etc/fstab ${PREFIX}/etc
	install etc/inetd.conf ${PREFIX}/etc
	install etc/inittab ${PREFIX}/etc
	install etc/init.d/rcS ${PREFIX}/etc/init.d
	install etc/init.d/wlan-start ${PREFIX}/etc/init.d
	-install etc/init.d/rcS.local ${PREFIX}/etc/init.d
	install etc/issue ${PREFIX}/etc
	install etc/protocols ${PREFIX}/etc
	install etc/services ${PREFIX}/etc
	install etc/hosts ${PREFIX}/etc
	install etc/nsswitch.conf ${PREFIX}/etc
	install etc/passwd ${PREFIX}/etc
	install etc/shadow ${PREFIX}/etc
	install etc/group ${PREFIX}/etc
	install ${LIB_DIR}/ld-*.so ${PREFIX}/lib
	install ${LIB_DIR}/ld-linux.so* ${PREFIX}/lib
	install ${LIB_DIR}/libc.* ${PREFIX}/lib
	install ${LIB_DIR}/libm.* ${PREFIX}/lib
	install ${LIB_DIR}/libdl.* ${PREFIX}/lib
	install ${LIB_DIR}/libnss_compat.* ${PREFIX}/lib
	install ${LIB_DIR}/libnss_dns.* ${PREFIX}/lib
	install ${LIB_DIR}/libnss_files.* ${PREFIX}/lib
	install ${LIB_DIR}/libresolv.* ${PREFIX}/lib
	install ${LIB_DIR}/libpthread.* ${PREFIX}/lib
	install ${LIB_DIR}/libutil.* ${PREFIX}/lib
	install ${LIB_DIR}/libnsl.* ${PREFIX}/lib
	install ${LIB_DIR}/libcrypt.* ${PREFIX}/lib
	install ${LIB_DIR}/libcrypt.* ${PREFIX}/lib
	install etc/dropbear/dropbear_dss_host_key ${PREFIX}/etc/dropbear
	install etc/dropbear/dropbear_rsa_host_key ${PREFIX}/etc/dropbear
	install etc/dropbear/README ${PREFIX}/etc/dropbear
	install etc/wpa_supplicant.conf ${PREFIX}/etc
	mkdir -p ${PREFIX}/etc/network/if-down.d
	mkdir -p ${PREFIX}/etc/network/if-post-down.d
	mkdir -p ${PREFIX}/etc/network/if-up.d
	mkdir -p ${PREFIX}/etc/network/if-pre-up.d
	install etc/network/interfaces ${PREFIX}/etc/network
	install etc/network/if_mapping ${PREFIX}/etc/network
	install etc/network/udhcpc_action ${PREFIX}/etc/network
	install etc/network/wpa_action ${PREFIX}/etc/network
