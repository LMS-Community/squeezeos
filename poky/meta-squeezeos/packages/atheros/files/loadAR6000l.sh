#!/bin/sh
# Loads and unloads the driver modules in the proper sequence
Help() {
    echo "Usage: $0 [options]"
    echo
    echo "With NO options, perform a default driver loading"
    echo
    echo "To bypass WMI: "
    echo "   $0 1"
    echo "To run Test commands:"
    echo "   $0 -t or $ --test"
    echo "To recover Flash using ROM BSP,ie,force target boot from ROM:"
    echo "   $0 -r or $0 --rom"
    echo " To enable console print:"
    echo "   $0 --enableuartprint"
    echo " To disable console print:"
    echo "   $0 --disableuartprint"
    echo " To specify the MAC address:"
    echo "   $0 --setmac xx:xx:xx:xx:xx:xx"
    echo " To specify the regulatory code:"
    echo "   $0 --setRD xx"
    echo " To specify the log file name:"
    echo "   $0 -l <log.file> or $0 --log <log.file>"
    echo "To see this help information:"
    echo "   $0 --help or $0 -h "
    echo "To unload all of the drivers:"
    echo "   $0 unloadall"
    exit 0
}

if [ -z "$WORKAREA" ]
then
    WORKAREA=.
fi

fromrom=0
slowbus=${slowbus:-0}
tmode=0
dounload=""
ar6000args=""
busclocksetting=""
bmi_enable=""
macaddr=""
setmacopt=""
regcode=""
regcodeopt=""
logfile="/tmp/dbglog.out"
NETIF=${NETIF:-eth1}
temp=""
START_WLAN="TRUE"
TARGET_ONLY="FALSE"


while [ "$#" -ne 0 ]
do
case $1 in
		-t|--test )
        tmode=1
        ar6000args="$ar6000args testmode=1"
        shift
        ;;
        -r|--rom )
        fromrom=1
        slowbus=1
        ar6000args="$ar6000args skipflash=1"
        shift
        ;;
        -l|--log )
        shift
        logfile="$1"
        shift
        ;;
        -h|--help )
        Help
        ;;
        --setmac )
        shift
        macaddr="$1"
        shift
        ;;
        --setRD )
        shift
        regcode="$1"
        shift
        ;;
        bmi|hostonly|-hostonly|--hostonly)
        bmi_enable="yes"
        shift
        ;;
        bypasswmi)
        ar6000args="$ar6000args bypasswmi=1"
        shift
        ;;
        noresetok)
        ar6000args="$ar6000args resetok=0"
        shift
        ;;
        specialmode)
        ar6000args="$ar6000args bypasswmi=1, resetok=0"
        bmi_enable="yes"
        shift
        ;;
        unloadall)
        dounload="yes"
        shift
        ;;
        enableuartprint)
        ar6000args="$ar6000args enableuartprint=1"
        shift
        ;;
        disableuartprint)
        ar6000args="$ar6000args enableuartprint=0"
        shift
        ;;
        logwmimsgs)
        ar6000args="$ar6000args logWmiRawMsgs=1"
        shift
        ;;
        --nostart|-nostart|nostart)
        START_WLAN="FALSE"
        shift
        ;;
	--targonly|-targonly|targonly)
	TARGET_ONLY="TRUE"
	shift
	;;
        * )
            echo "Unsupported argument"
            exit -1
        shift
    esac
done

export IMAGEPATH=$WORKAREA

# If "targonly" was specified on the command line, then don't
# load (or unload) the host driver.  Skip to target-side setup.
if [ "$TARGET_ONLY" = "FALSE" ]
then # {
	if [ "$dounload" = "yes" ]
	then
		echo "..unloading all"
		/sbin/rmmod -w ar6000.ko
		killall recEvent
		exit 0
	fi

# For AR6002 FPGA rather than silicon, uncomment these lines:
# wlanapp=${wlanapp:-$WORKAREA/target/AR6002/fpga/bin/athwlan.bin}
# slowbus=1
# RPDF_FILE=$WORKAREA/target/AR6002/fpga/bin/patch.rpdf

	$IMAGEPATH/recEvent $logfile > /dev/null 2>&1 &
	/sbin/insmod $IMAGEPATH/ar6000.ko bmienable=1 $ar6000args busspeedlow=$slowbus
fi # }

# At this point, all Host-side software is loaded.
# So we turn our attention to Target-side setup.


# If user requested BMI mode, he will handle everything himself.
# Otherwise, we load the Target firmware, load EEPROM and exit
# BMI mode.
exit_value=0
if [ "$bmi_enable" != "yes" ] # {
then

    if [ -e "/sys/module/ar6000/debugbmi" ]; then
        PARAMETERS="/sys/module/ar6000"
    fi

    if [ -e "/sys/module/ar6000/parameters/debugbmi" ]; then
        PARAMETERS="/sys/module/ar6000/parameters"
    fi

    if [ "$PARAMETERS" != "" ]; then
        # For speed, temporarily disable Host-side debugging
        save_dbg=`cat ${PARAMETERS}/debugdriver`
        echo 0 > ${PARAMETERS}/debugdriver
    fi
    
    if [ "$TARGET_TYPE" = "" ]
    then
        # Determine TARGET_TYPE
        eval export `$IMAGEPATH/bmiloader --timeout=2 -i $NETIF --quiet --info | grep TARGET_TYPE`
    fi
    #echo TARGET TYPE is $TARGET_TYPE

    if [ "$TARGET_TYPE" = "" ]
    then
        echo "Failed to get TARGET TYPE"
        exit -1
    fi

    if [ "$TARGET_VERSION" = "" ]
    then
        # Determine TARGET_VERSION
        eval export `$IMAGEPATH/bmiloader -i $NETIF --quiet --info | grep TARGET_VERSION`
    fi

    if [ "$TARGET_VERSION" = "" ]
    then
        echo "Failed to get TARGET VERSION"
        exit -1
    fi

    AR6002_VERSION_REV1=0x20000086
    AR6002_VERSION_REV2=0x20000188
    # echo TARGET VERSION is $TARGET_VERSION

    if [ "$TARGET_TYPE" = "AR6002" ]
    then
        # Temporarily disable System Sleep on AR6002
        old_options=`$IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=0x180c0 --or=8`
        old_sleep=`$IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=0x40c4 --or=1`

        if [ "$TARGET_VERSION" = "$AR6002_VERSION_REV1" ]
        then
            # TBDXXX: Eventually desupport 1.x and remove workaround
            $WORKAREA/host/support/AR6002.war1.sh > /dev/null

            # TBDXXX TEMPORARY Silicon 1.0 WORKAROUND: Use external clock for RTC (set LF_CLK32 bit)
            $IMAGEPATH/bmiloader -i $NETIF --set --address=0x4028 --param=5 > /dev/null
        else
            # AR6002 REV2 clock setup.

	    # Run at 40/44MHz by default.
            $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=0x4020 --param=0

            # It is the Host's responsibility to set hi_refclk_hz
            # according to the actual speed of the reference clock.
            #
            # hi_refclk_hz to 26MHz
            # (Commented out because 26MHz is the default.)
            # $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=0x500478 --param=26000000

            # If hi_ext_clk_detected is 0, arrange to use internal clock
            ext_clk_detected=`$IMAGEPATH/bmiloader -i $NETIF --quiet --get --address=0x50047c`
            if [ $ext_clk_detected = 0x0 ]
            then
                # LPO_CAL_TIME for 26MHz
                # param=27500/Actual_MHz_of_refclk
                # (Commented out because 26MHz is the default.)
                # $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=0x40d4 --param=1057

                # LPO_CAL.ENABLE = 1
                $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=0x40e0 --param=0x100000
            fi

            # Set XTAL_SETTLE to ~2Ms default; some crystals may require longer settling time.
            # In general, XTAL_SETTLE time is handled by the WLAN driver (because it needs
            # to match the MAC's SLEEP32_WAKE_XTL_TIME).  The driver's default is 2Ms; so for
            # typical use there's no need to set it here.
            # param=ceiling(DesiredSeconds*32768)
            # $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=0x401c --param=66
        fi
    fi

    if [ "$RPDF_FILE" = "" ]
    then
        # Default value for ROM Patch Distribution file
        # is based on Target type and revision.
        if [ "$TARGET_TYPE" = "AR6001" ]
        then
            RPDF_FILE=$WORKAREA/target/AR6001/bin/patch.rpdf
        else
          if [ "$TARGET_VERSION" = "$AR6002_VERSION_REV1" ]
          then
            RPDF_FILE=$WORKAREA/target/AR6002/hw1.0/bin/patch.rpdf
          else
###//??            RPDF_FILE=$WORKAREA/target/AR6002/hw2.0/bin/patch.rpdf
            RPDF_FILE=$WORKAREA/patch.rpdf
          fi
        fi
    fi

    if [ "$RPDF_FILE" != "" -a -r "$RPDF_FILE" ]
    then
        echo "Install ROM Patch Distribution File, " $RPDF_FILE
        $IMAGEPATH/fwpatch --interface=$NETIF --file=$RPDF_FILE
        if [ $? -ne 0 ]
        then
            echo "Failed to load ROM Patch Distribution"
            exit_value=-1
        fi
    fi

    if [ "$EEPROM" != "" ]
    then
        echo "Load Board Data from $EEPROM"

        if [ "$regcode" != "" ]
        then
            regcodeopt="--setRD=$regcode"
            echo "Updating regulatory code"
        fi
        if [ "$macaddr" != "" ]
        then
            setmacopt="--setmac=$macaddr"
            echo "Updating MAC address"
        fi
        $IMAGEPATH/eeprom.$TARGET_TYPE --transfer --file=$EEPROM $setmacopt $regcodeopt --interface=$NETIF
        if [ $? -ne 0 ]
        then
            echo "Failed to load Board Data from file."
            exit_value=-1
        fi
    else
        # Transfer Board Data from Target EEPROM to Target RAM
        # If EEPROM does not appear valid, this has no effect.
        $IMAGEPATH/eeprom.$TARGET_TYPE --transfer --interface=$NETIF 2> /dev/null

        # On AR6001, a failure to load from EEPROM is not fatal
        # because Board Data may be in flash.
        # On a standard driver load of AR6002, lack of Board
        # Data is fatal.
        if [ \( $? -ne 0 \) -a \( "$TARGET_TYPE" = "AR6002" \) ]
        then
            echo "Failed to load Board Data from EEPROM."
            exit_value=-1
        fi
    fi

    # For AR6002, download athwlan.bin to Target RAM.
    # For AR6001, athwlan is already in ROM or Flash.
    if [ "$TARGET_TYPE" = "AR6002" ]
    then
        # Download the Target application, usually athwlan.bin,
        # into Target RAM.
        if [ "$TARGET_VERSION" = "$AR6002_VERSION_REV1" ]
        then
            LOADADDR=0x502000
###//??            wlanapp=${wlanapp:-$WORKAREA/target/AR6002/hw1.0/bin/athwlan.bin}
            wlanapp=${wlanapp:-$WORKAREA/athwlan.bin}
        else
            LOADADDR=0x502400
###//??            wlanapp=${wlanapp:-$WORKAREA/target/AR6002/hw2.0/bin/athwlan.bin}
            wlanapp=${wlanapp:-$WORKAREA/athwlan.bin}
        fi

        if [ -r $wlanapp.z77 ]
        then
            # If a compressed version of the WLAN application exists, use it
            $IMAGEPATH/bmiloader -i $NETIF --write --address=$LOADADDR --file=$wlanapp.z77 --uncompress
        else
            $IMAGEPATH/bmiloader -i $NETIF --write --address=$LOADADDR --file=$wlanapp
        fi

        # WLAN Initialization DataSets
        if [ "$TARGET_VERSION" = "$AR6002_VERSION_REV1" ]
        then
            $IMAGEPATH/bmiloader -i $NETIF --write --address=0x513c90 --file=$WORKAREA/target/AR6002/hw1.0/bin/data.patch.hw1_0.bin
           $IMAGEPATH/bmiloader -i $NETIF --write --address=0x500418 --param=0x513c90
        else
###//??            $IMAGEPATH/bmiloader -i $NETIF --write --address=0x52d8bc --file=$WORKAREA/target/AR6002/hw2.0/bin/data.patch.hw2_0.bin
            $IMAGEPATH/bmiloader -i $NETIF --write --address=0x52d8bc --file=$WORKAREA/data.patch.hw2_0.bin
            $IMAGEPATH/bmiloader -i $NETIF --write --address=0x500418 --param=0x52d8bc
        fi

        # Restore System Sleep on AR6002
        $IMAGEPATH/bmiloader -i $NETIF --set --address=0x40c4 --param=$old_sleep > /dev/null
        $IMAGEPATH/bmiloader -i $NETIF --set --address=0x180c0 --param=$old_options > /dev/null
        
        # reset hi_system_sleep_setting, for REV 1 silicon.  This undoes the WAR performed in the
        # driver that immediately disables sleep.
        if [ "$TARGET_VERSION" = "$AR6002_VERSION_REV1" ]
        then
        	$IMAGEPATH/bmiloader -i $NETIF --write --address=0x50042c --param=0 > /dev/null
        fi
        
	# Enable HI_OPTION_TIMER_WAR (timerwar)
	$IMAGEPATH/bmiloader -i $NETIF --set --address=0x500410 --or=1
    fi

    if [ "$TARGET_TYPE" = "AR6001" ]
    then
        # Request maximum system performance (max clock rates)
        $IMAGEPATH/bmiloader -i $NETIF --set --address=0xac000020 --param=0x203 > /dev/null
        $IMAGEPATH/bmiloader -i $NETIF --set --address=0xac000024 --param=0x203 > /dev/null
    fi
    
    if [ "$PARAMETERS" != "" ]; then
        # Restore Host debug state.
        echo $save_dbg > ${PARAMETERS}/debugdriver
    fi

    if [ \( $exit_value -eq 0 \) -a \( $START_WLAN = TRUE \) ]
    then
        # Leave BMI now and start the WLAN driver.
        $IMAGEPATH/bmiloader -i $NETIF --done
    fi
fi # }

exit $exit_value
