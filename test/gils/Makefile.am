# $Id: Makefile.am,v 1.16 2003-09-24 11:45:44 adam Exp $

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

check_SCRIPTS = stop01.sh stop02.sh stop03.sh stop04.sh \
        test1.sh test2.sh timing1.sh timing2.sh

TESTS = $(check_SCRIPTS)

EXTRA_DIST = zebra1.cfg zebra2.cfg $(check_SCRIPTS)
