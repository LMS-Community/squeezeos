privatedatadir = $(pkgdatadir)
privatedata_DATA = oprofile-viewer.glade

applicationsdir = $(datadir)/applications
applications_DATA = oprofile-viewer.desktop

EXTRA_DIST = \
	$(privatedata_DATA) \
	$(applications_DATA)\
	oprofile-viewer-16.png oprofile-viewer-22.png \
	oprofile-viewer-24.png oprofile-viewer-32.png \
	oprofile-viewer-48.png oprofile-viewer.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-icons:
	for size in 16x16 22x22 24x24 32x32 48x48 scalable; do mkdir --parents $(DESTDIR)/$(datadir)/icons/hicolor/$$size/apps; done
	install -m 644 $(srcdir)/oprofile-viewer-16.png $(DESTDIR)/$(datadir)/icons/hicolor/16x16/apps/oprofile-viewer.png
	install -m 644 $(srcdir)/oprofile-viewer-22.png $(DESTDIR)/$(datadir)/icons/hicolor/22x22/apps/oprofile-viewer.png
	install -m 644 $(srcdir)/oprofile-viewer-24.png $(DESTDIR)/$(datadir)/icons/hicolor/24x24/apps/oprofile-viewer.png
	install -m 644 $(srcdir)/oprofile-viewer-32.png $(DESTDIR)/$(datadir)/icons/hicolor/32x32/apps/oprofile-viewer.png
	install -m 644 $(srcdir)/oprofile-viewer-48.png $(DESTDIR)/$(datadir)/icons/hicolor/48x48/apps/oprofile-viewer.png
	install -m 644 $(srcdir)/oprofile-viewer.svg $(DESTDIR)/$(datadir)/icons/hicolor/scalable/apps/oprofile-viewer.svg

install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

install-data-local: install-icons

uninstall-icons:
	rm -f $(DESTDIR)/$(datadir)/icons/hicolor/16x16/apps/oprofile-viewer.png
	rm -f $(DESTDIR)/$(datadir)/icons/hicolor/22x22/apps/oprofile-viewer.png
	rm -f $(DESTDIR)/$(datadir)/icons/hicolor/24x24/apps/oprofile-viewer.png
	rm -f $(DESTDIR)/$(datadir)/icons/hicolor/32x32/apps/oprofile-viewer.png
	rm -f $(DESTDIR)/$(datadir)/icons/hicolor/48x48/apps/oprofile-viewer.png
	rm -f $(DESTDIR)/$(datadir)/icons/hicolor/scalable/apps/oprofile-viewer.svg

uninstall-local: uninstall-icons

