## Process this file with automake to produce Makefile.in

if ENABLE_SERVER
SERVER_DIR = server
endif

if ENABLE_CLIENT
CLIENT_DIR = src data scripts
endif

SUBDIRS = protocol $(SERVER_DIR) $(CLIENT_DIR)
DIST_SUBDIRS = protocol server src data scripts

doc_DATA = \
	README \
	COPYING \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS

EXTRA_DIST = $(doc_DATA)
