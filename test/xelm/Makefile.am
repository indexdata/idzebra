# $Id: Makefile.am,v 1.3 2003-05-21 14:39:22 adam Exp $

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.xml $(distdir)/records
	-mkdir $(distdir)/tmp
	-mkdir $(distdir)/lock
	-mkdir $(distdir)/register

check_SCRIPTS = test.sh

TESTS = $(check_SCRIPTS)

EXTRA_DIST = zebra.cfg user.abs $(check_SCRIPTS)

