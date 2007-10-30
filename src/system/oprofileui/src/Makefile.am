## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(OPROFILEUI_CFLAGS) $(AVAHI_CFLAGS) \
	-I$(top_srcdir)/protocol/

AM_CFLAGS =\
	 -Wall\
	 -g \
	 -std=gnu99 \
	-DPKG_DATA_DIR=\""$(pkgdatadir)"\" 


bin_PROGRAMS = oprofile-viewer

noinst_PROGRAMS = testparser

oprofile_viewer_SOURCES = \
	main.c \
	main.h \
	client.c \
	client.h \
	archive.c \
	archive.h \
	archive_window.c \
	archive_window.h \
	archive_save_window.c \
	archive_save_window.h \
	view.c \
	view.h \
	report.c \
	report.h \
	parser.c \
	parser.h \
	image.c \
	image.h \
	symbol.c \
	symbol.h \
	module.c \
	module.h \
	symbol_instance.c \
	symbol_instance.h \
	module_instance.c \
	module_instance.h \
	callee_symbol_instance.c \
	callee_symbol_instance.h \
	caller_symbol_instance.c \
	callee_symbol_instance.h \
	types.h

oprofile_viewer_LDFLAGS = 

oprofile_viewer_LDADD = $(OPROFILEUI_LIBS) $(AVAHI_LIBS) $(top_builddir)/protocol/libop.a

if WITH_AVAHI
oprofile_viewer_SOURCES += avahi-ui.c avahi-ui.h
endif

testparser_SOURCES = \
	parser.c \
	parser.h \
	testparser.c \
	report.c \
	report.h \
	image.c \
	image.h \
	symbol.c \
	symbol.h \
	module.c \
	module.h \
	symbol_instance.c \
	symbol_instance.h \
	module_instance.c \
	module_instance.h \
	callee_symbol_instance.c \
	callee_symbol_instance.h \
	caller_symbol_instance.c \
	caller_symbol_instance.h \
	types.h

testparser_LDFLAGS = 

testparser_LDADD = $(OPROFILEUI_LIBS)
