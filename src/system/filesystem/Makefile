ARCH=arm-926ejs-linux-gnueabi
LIB_DIR=../../../toolchain/gcc-4.2.2-glibc-2.6.1/$(ARCH)/$(ARCH)/lib

install:
	mkdir -p ${PREFIX} ${PREFIX}/dev ${PREFIX}/etc ${PREFIX}/etc/init.d ${PREFIX}/bin ${PREFIX}/lib ${PREFIX}/proc ${PREFIX}/sys ${PREFIX}/mnt ${PREFIX}/mnt/storage ${PREFIX}/mnt/overlay ${PREFIX}/mnt/mmc ${PREFIX}/tmp ${PREFIX}/var ${PREFIX}/var/shm ${PREFIX}/var/log ${PREFIX}/var/run ${PREFIX}/etc/network ${PREFIX}/etc/dropbear ${PREFIX}/usr/lib ${PREFIX}/usr/bin ${PREFIX}/root
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
	install etc/motd ${PREFIX}/etc
	install etc/mdev.conf ${PREFIX}/etc
	cd ${PREFIX}/etc; ln -sf /proc/mounts mtab
	install ${LIB_DIR}/ld-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf ld-2.6.1.so ld-linux.so.3
	install ${LIB_DIR}/libcrypt-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libcrypt-2.6.1.so libcrypt.so.1 
	cd ${PREFIX}/lib; ln -sf libcrypt.so.1 libcrypt.so
	install ${LIB_DIR}/libc.so ${PREFIX}/lib
	install ${LIB_DIR}/libc-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libc-2.6.1.so libc.so.6
	install ${LIB_DIR}/libdl-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libdl-2.6.1.so libdl.so.2
	cd ${PREFIX}/lib; ln -sf libdl.so.2 libdl.so
	install ${LIB_DIR}/libm-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libm-2.6.1.so libm.so.6
	cd ${PREFIX}/lib; ln -sf libm.so.6 libm.so
	install ${LIB_DIR}/libnsl-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libnsl-2.6.1.so libnsl.so.1
	cd ${PREFIX}/lib; ln -sf libnsl.so.1 libnsl.so
	install ${LIB_DIR}/libnss_compat-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libnss_compat-2.6.1.so libnss_compat.so.2
	cd ${PREFIX}/lib; ln -sf libnss_compat.so.2 libnss_compat.so
	install ${LIB_DIR}/libnss_dns-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libnss_dns-2.6.1.so libnss_dns.so.2
	cd ${PREFIX}/lib; ln -sf libnss_dns.so.2 libnss_dns.so
	install ${LIB_DIR}/libnss_files-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libnss_files-2.6.1.so libnss_files.so.2
	cd ${PREFIX}/lib; ln -sf libnss_files.so.2 libnss_files.so
	install ${LIB_DIR}/libpthread.so.0 ${PREFIX}/lib
	install ${LIB_DIR}/libpthread-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libpthread-2.6.1.so libpthread.so.0
	install ${LIB_DIR}/libresolv-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libresolv-2.6.1.so libresolv.so.2
	cd ${PREFIX}/lib; ln -sf libresolv.so.2 libresolv.so
	install ${LIB_DIR}/librt-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf librt-2.6.1.so librt.so.1
	cd ${PREFIX}/lib; ln -sf librt.so.1 librt.so
	install ${LIB_DIR}/libutil-2.6.1.so ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libutil-2.6.1.so libutil.so.1
	cd ${PREFIX}/lib; ln -sf libutil.so.1 libutil.so
	install ${LIB_DIR}/libgcc_s.so.1 ${PREFIX}/lib
	cd ${PREFIX}/lib; ln -sf libgcc_s.so.1 libgcc_s.so
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
