#!/bin/sh
#
# Called from udev
#
# Attempt to mount any added block devices and umount any removed devices


MOUNT="/bin/mount -t auto"
PMOUNT="/usr/bin/pmount"
UMOUNT="/bin/umount"
MOUNT_OPTIONS=""

for line in `grep -v ^# /etc/udev/mount.blacklist`
do
	if { echo "$DEVNAME" | grep -q "$line" ; }
	then
		logger "udev/mount.sh" "[$DEVNAME] is blacklisted, ignoring"
		exit 0
	fi
done

automount() {	
	name="`basename "$DEVNAME"`"
	fstype="`/lib/udev/vol_id --type "$DEVNAME"`"

	! test -d "/media/$name" && mkdir -p "/media/$name"
	
	if [ "$fstype" = "ntfs" ]; then
		MOUNT="/usr/bin/ntfs-3g"
	elif [ "$fstype" = "vfat" ]; then
		# See http://lxr.linux.no/linux/Documentation/filesystems/vfat.txt
		MOUNT_OPTIONS="-o rw,fmask=0022,dmask=0022,codepage=437,iocharset=iso8859-1,utf8=1"
	fi
	
	if ! $MOUNT $DEVNAME "/media/$name" $MOUNT_OPTIONS
	then
		#logger "mount.sh/automount" "$MOUNT $DEVNAME \"/media/$name\" failed!"
		rm_dir "/media/$name"
	else
		logger "mount.sh/automount" "Auto-mount of [/media/$name] successful"
		touch "/tmp/.automount-$name"
	fi
}
	
rm_dir() {
	# We do not want to rm -r populated directories
	if test "`find "$1" | wc -l | tr -d " "`" -lt 2 -a -d "$1"
	then
		! test -z "$1" && rm -r "$1"
	else
		logger "mount.sh/automount" "Not removing non-empty directory [$1]"
	fi
}

if [ "$ACTION" = "add" ] && [ -n "$DEVNAME" ]; then
	if [ -x "$PMOUNT" ]; then
		$PMOUNT $DEVNAME 2> /dev/null
	elif [ -x $MOUNT ]; then
    		$MOUNT $DEVNAME 2> /dev/null
	fi
	
	# If the device isn't mounted at this point, it isn't configured in fstab
	grep -q "^$DEVNAME " /proc/mounts || automount

	# Restart samba
	/etc/init.d/samba restart
fi



if [ "$ACTION" = "remove" ] && [ -x "$UMOUNT" ] && [ -n "$DEVNAME" ]; then
	for mnt in `cat /proc/mounts | grep "$DEVNAME" | cut -f 2 -d " " `
	do
		$UMOUNT $mnt
	done
	
	# Remove empty directories from auto-mounter
	name="`basename "$DEVNAME"`"
	test -e "/tmp/.automount-$name" && rm_dir "/media/$name"

	# Restart samba
	/etc/init.d/samba restart
fi
