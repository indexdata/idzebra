# $Id: Makefile.am,v 1.2 2003-05-20 13:52:41 adam Exp $

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.xml $(distdir)/records
	-mkdir $(distdir)/tmp
	-mkdir $(distdir)/lock
	-mkdir $(distdir)/register

TESTS = test.sh

EXTRA_DIST = $(TESTS) zebra.cfg user.abs

