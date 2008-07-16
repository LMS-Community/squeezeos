DESCRIPTION = "Task for SqueezeOS - minimal bootable image"
PACKAGE_ARCH = "${MACHINE_ARCH}"
DEPENDS = "virtual/kernel"
ALLOW_EMPTY = "1"
PR = "r19"

inherit task

PROVIDES = "${PACKAGES}"
PACKAGES = " \
    task-squeezeos-base \
    "

#
# task-base contain stuff needed for base system (machine related)
#
RDEPENDS_task-squeezeos-base = "\
    squeezeos-base-files \
    busybox \
    "
