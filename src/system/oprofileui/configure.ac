dnl Process this file with autoconf to produce a configure script.

AC_INIT(oprofileui, 0.1.1)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_RANLIB

dnl Set gettext package name
GETTEXT_PACKAGE=oprofileui
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

dnl Add the languages which your application supports here.
ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT

AC_ARG_ENABLE(server,
            [AC_HELP_STRING([--enable-server], 
            [Build the server])], enable_server="$enableval", enable_server=yes)

AC_ARG_ENABLE(client,
            [AC_HELP_STRING([--enable-client], 
            [Build the client])], enable_client="$enableval", enable_client=yes)

AM_CONDITIONAL(ENABLE_SERVER, test x$enable_server = xyes)
AM_CONDITIONAL(ENABLE_CLIENT, test x$enable_client = xyes)

PKG_CHECK_MODULES(OPROFILEUI, [glib-2.0 libglade-2.0 gtk+-2.0 libxml-2.0 gnome-vfs-2.0 gconf-2.0])
AC_SUBST(OPROFILEUI_CFLAGS)
AC_SUBST(OPROFILEUI_LIBS)

PKG_CHECK_MODULES(OPROFILE_SERVER, [glib-2.0])
AC_SUBST(OPROFILE_SERVER_CFLAGS)
AC_SUBST(OPROFILE_SERVER_LIBS)

AC_ARG_WITH(avahi,
        [AC_HELP_STRING([--with-avahi], [Use Avahi to announce and search for OProfile servers])],
        with_avahi="$withval", with_avahi=no)
if test $with_avahi = "yes"; then
        PKG_CHECK_MODULES(AVAHI, [avahi-client avahi-glib])
        AC_DEFINE_UNQUOTED(WITH_AVAHI, [1], [Using Avahi])
fi
AM_CONDITIONAL(WITH_AVAHI, test x$with_avahi = xyes)

AC_OUTPUT([
Makefile
protocol/Makefile
server/Makefile
src/Makefile
data/Makefile
scripts/Makefile
])

echo
echo "Using Avahi:     $with_avahi"
echo "Building server: $enable_server"
echo "Building client: $enable_client"
echo
