#!/bin/sh

case "$1" in
    start)
	if [ -s /var/run/monitor_msp430.pid ] && kill -0 $(cat /var/run/monitor_msp430.pid)
	then
		echo "Monitor is already running"
		exit 1
	fi

	echo "Monitoring MSP430"

	(cd /usr/bin; /usr/sbin/monitor_msp430) &
	echo $! > /var/run/monitor_msp430.pid
	;;

    stop)
	echo "Stop monitoring the MSP430"

	if [ -s /var/run/monitor_msp430.pid ]; then
		/bin/kill $(cat /var/run/monitor_msp430.pid)

		/bin/rm /var/run/monitor_msp430.pid
		/bin/rm /var/run/monitor_msp430.wdog
	fi
	;;
    restart)
	$0 stop
	$0 start
	;;

    *)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit 0
