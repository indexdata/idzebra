
EXTRA_DIST = zebra.cfg test.sh

dist-hook:
	mkdir -p $(distdir)/records
	mkdir -p $(distdir)/tab
	cp -pR $(srcdir)/records/* $(distdir)/records
	cp -pR $(srcdir)/tab/* $(distdir)/tab
	mkdir -p $(distdir)/shadow
	mkdir -p $(distdir)/register
	mkdir -p $(distdir)/lock
	mkdir -p $(distdir)/tmp
