
gilsdatadir=$(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)-examples/gils
	
gilsdata_DATA = zebra.cfg

EXTRA_DIST = $(gilsdata_DATA)

clean:
	rm -f *.mf *.LCK zebrasrv.pid

install-data-hook:
	test -z "$(DESTDIR)$(gilsdatadir)/records" || $(MKDIR_P) "$(DESTDIR)$(gilsdatadir)/records"
	for p in $(srcdir)/records/*.grs; do \
                $(INSTALL_DATA) $$p $(DESTDIR)$(gilsdatadir)/records; \
        done

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records
