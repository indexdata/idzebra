# $Id: Makefile.am,v 1.3 2004-03-09 18:45:17 adam Exp $

check_SCRIPTS = test1.sh

TESTS = $(check_SCRIPTS)

EXTRA_DIST = zebra.cfg test.sh $(check_SCRIPTS)

dist-hook:
	mkdir -p $(distdir)/records
	mkdir -p $(distdir)/tab
	-cp -p $(srcdir)/records/* $(distdir)/records
	-cp -p $(srcdir)/tab/* $(distdir)/tab
	mkdir -p $(distdir)/shadow
	mkdir -p $(distdir)/register
	mkdir -p $(distdir)/lock
	mkdir -p $(distdir)/tmp
