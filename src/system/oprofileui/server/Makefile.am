INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(OPROFILE_SERVER_CFLAGS) \
	-I$(top_srcdir)/protocol/

AM_CFLAGS =\
	 -Wall\
	 -g \
	 -std=c99 \
	-DPKG_DATA_DIR=\""$(pkgdatadir)"\" 

bin_PROGRAMS = oprofile-server

oprofile_server_SOURCES = opserver.c
oprofile_server_LDFLAGS = -lm
oprofile_server_LDADD = \
	$(OPROFILE_SERVER_LIBS) \
	$(top_builddir)/protocol/libop.a

if WITH_AVAHI
oprofile_server_SOURCES += mdns.h mdns.c
INCLUDES += $(AVAHI_CFLAGS)
oprofile_server_LDADD += $(AVAHI_LIBS)
endif
