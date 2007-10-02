ARCH=arm-9tdmi-linux-gnu
LIB_DIR=../../../toolchain/gcc-4.1.1-glibc-2.3.2/$(ARCH)/$(ARCH)/lib

install:
	mkdir -p ${PREFIX} ${PREFIX}/dev ${PREFIX}/etc ${PREFIX}/etc/init.d ${PREFIX}/bin ${PREFIX}/lib ${PREFIX}/proc ${PREFIX}/sys ${PREFIX}/mnt ${PREFIX}/mnt/yaffs ${PREFIX}/mnt/overlay ${PREFIX}/mnt/mmc ${PREFIX}/tmp ${PREFIX}/var ${PREFIX}/var/shm ${PREFIX}/var/log ${PREFIX}/var/run ${PREFIX}/etc/network ${PREFIX}/etc/dropbear ${PREFIX}/usr/lib ${PREFIX}/usr/bin ${PREFIX}/root
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
	cd ${PREFIX}/etc; ln -sf /proc/mounts mtab
	#install ${LIB_DIR}/ld-*.so* ${PREFIX}/lib
	#install ${LIB_DIR}/ld-linux.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libcrypt.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libc.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libdl.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libm.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libnsl.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libnss_compat.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libnss_dns.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libnss_files.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libpthread.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libresolv.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libstdc++.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libutil.so* ${PREFIX}/lib
	#install ${LIB_DIR}/libgcc_s.so* ${PREFIX}/lib
	install ${LIB_DIR}/ld-2.3.2.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf ld-2.3.2.so ld-linux.so.2
	install ${LIB_DIR}/libcrypt.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libcrypt.so libcrypt.so.1
	install ${LIB_DIR}/libc.so ${PREFIX}/lib
	install ${LIB_DIR}/libc.so.6 ${PREFIX}/lib
	install ${LIB_DIR}/libdl.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libdl.so libdl.so.2
	install ${LIB_DIR}/libm.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libm.so libm.so.6
	install ${LIB_DIR}/libnsl.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libnsl.so libnsl.so.1
	install ${LIB_DIR}/libnss_compat.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libnss_compat.so libnss_compat.so.2
	install ${LIB_DIR}/libnss_dns.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libnss_dns.so libnss_dns.so.2
	install ${LIB_DIR}/libnss_files.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libnss_files.so libnss_files.so.2
	install ${LIB_DIR}/libpthread.so ${PREFIX}/lib
	install ${LIB_DIR}/libpthread.so.0 ${PREFIX}/lib
	install ${LIB_DIR}/libresolv.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libresolv.so libresolv.so.2
	install ${LIB_DIR}/libutil.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libutil.so libutil.so.1
	install ${LIB_DIR}/libgcc_s.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libgcc_s.so libgcc_s.so.1
	install etc/dropbear/dropbear_dss_host_key ${PREFIX}/etc/dropbear
	install etc/dropbear/dropbear_rsa_host_key ${PREFIX}/etc/dropbear
	install etc/dropbear/README ${PREFIX}/etc/dropbear
	install etc/wpa_supplicant.conf ${PREFIX}/etc
	mkdir -p ${PREFIX}/etc/network/if-down.d
	mkdir -p ${PREFIX}/etc/network/if-post-down.d
	mkdir -p ${PREFIX}/etc/network/if-up.d
	mkdir -p ${PREFIX}/etc/network/if-pre-up.d
	install etc/network/config ${PREFIX}/etc/network
	install etc/network/interfaces ${PREFIX}/etc/network
	install etc/network/if_mapping ${PREFIX}/etc/network
	install etc/network/udhcpc_action ${PREFIX}/etc/network
	install etc/network/wpa_action ${PREFIX}/etc/network
