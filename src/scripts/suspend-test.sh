#!/bin/sh

count=0
sleepfor=10

while [ /bin/true ]; do
    echo "--------------- Test round $count ----------------"

    echo $count: suspend for $sleepfor
    /etc/init.d/suspend $sleepfor

    echo $count: waiting for wireless connection
    while ! `wpa_cli status | grep -q COMPLETED`; do echo "wait wlan"; done

    date
    echo $count: waiting for dhcp
    while ! `wpa_cli status | grep -q 10.1.1.`; do echo "wait dhcp"; done

    date
    iwconfig eth0

    ping -c 10 10.1.1.10
    if [ $? -ne 0 ]; then
	exit 1
    fi

    cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
    echo 50000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed

    ping -c 10 10.1.1.10
    if [ $? -ne 0 ]; then
	exit 1
    fi

    cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
    echo 200000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed

    count=`expr $count + 1`
    sleepfor=`expr $sleepfor + 1`

    if [ $sleepfor -gt 20 ]; then
	echo resetting sleep count
	sleepfor=10
    fi
done