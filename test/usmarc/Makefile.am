
EXTRA_DIST = zebra.cfg test.sh

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*marc* $(distdir)/records
	mkdir $(distdir)/shadow
	mkdir $(distdir)/register
	mkdir $(distdir)/lock
	mkdir $(distdir)/tmp
