# $Id: Makefile.am,v 1.14 2003-05-20 13:52:41 adam Exp $

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

TESTS = stop01.sh stop02.sh stop03.sh stop04.sh \
        test1.sh test2.sh \
	timing1.sh timing2.sh

EXTRA_DIST = $(TESTS) zebra1.cfg zebra2.cfg
