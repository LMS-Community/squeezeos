#!/bin/sh
#
# Linux Native MMC-SDIO stack platform setup script
#
#

case $1 in
	loadbus)
	;;

	unloadbus)
	# nothing to do for native MMC stack
	;;
	
	loadAR6K)
####	$IMAGEPATH/recEvent $AR6K_TGT_LOGFILE /dev/null 2>&1 &
	/sbin/insmod $IMAGEPATH/$AR6K_MODULE_NAME.ko $AR6K_MODULE_ARGS debugflags=0x120 debughtc=0xffffffff 
	if [ $? -ne 0 ]; then
		echo "*** Failed to install AR6K Module: $AR6K_MODULE_ARGS"
		exit -1
	fi
	;;
	
	unloadAR6K)
	/sbin/rmmod -w $AR6K_MODULE_NAME.ko
#### 	killall recEvent
	;;
	*)
		echo "Unknown option : $1"
	
esac

