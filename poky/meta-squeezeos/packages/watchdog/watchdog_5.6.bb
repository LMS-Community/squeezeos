DESCRIPTION = "System watchdog daemon"
LICENSE = "GPL"
PR = "r5"

SRC_URI = " \
	http://www.ibiblio.org/pub/Linux/system/daemons/watchdog/${PN}-${PV}.tar.gz \
	file://memory-buffers-cache.patch;patch=1 \
	file://watchdog-semaphore.patch;patch=1 \
	file://startup-delay.patch;patch=1 \
	"

inherit autotools

FILES_${PN} = "${sbindir}/watchdog"
