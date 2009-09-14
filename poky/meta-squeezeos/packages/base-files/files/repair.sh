#!/bin/sh

# Log system info
/usr/bin/logger -p 2 watchdog error $1
/usr/bin/logger -p 5 $(/bin/cat /etc/squeezeos.version)
/usr/bin/logger -p 5 $(/usr/bin/uptime)

# Show processes
/usr/bin/psall | /usr/bin/logger

# Request crash from squeezeplay
if [ -s /var/run/squeezeplay.pid ]; then
	kill -3 $(cat /var/run/squeezeplay.pid)
fi

# Delay for syslog
sleep 2

# Copy of syslog to flash
CRASHLOG=$(mktemp /root/crashlog.XXXXXX)
/bin/cp /var/log/messages $CRASHLOG

#/bin/sync

# Let watchdog do it's work
exit $1
