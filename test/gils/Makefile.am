# $Id: Makefile.am,v 1.17 2004-01-30 11:43:41 heikki Exp $

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

check_SCRIPTS = stop01.sh stop02.sh stop03.sh stop04.sh \
        test1.sh test2.sh test3.sh timing1.sh timing2.sh

TESTS = $(check_SCRIPTS)

EXTRA_DIST = zebra1.cfg zebra2.cfg $(check_SCRIPTS)
