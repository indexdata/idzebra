# $Id: Makefile.am,v 1.4 2004-03-09 18:52:37 adam Exp $

check_SCRIPTS = test1.sh

TESTS = $(check_SCRIPTS)

EXTRA_DIST = zebra.cfg test.sh $(check_SCRIPTS)

dist-hook:
	mkdir -p $(distdir)/records
	mkdir -p $(distdir)/tab
	-cp -p $(srcdir)/records/* $(distdir)/records
	-cp -p $(srcdir)/tab/* $(distdir)/tab
