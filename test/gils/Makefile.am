# $Id: Makefile.am,v 1.15 2003-05-21 14:39:22 adam Exp $

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

# stop03.sh only works when Zebra is using threads.
if ISTHR
testscripts_thread = stop03.sh
else
testscripts_thread = 
endif

check_SCRIPTS = stop01.sh stop02.sh stop03.sh stop04.sh \
        test1.sh test2.sh timing1.sh timing2.sh

TESTS = stop01.sh stop02.sh $(testscripts_thread) stop04.sh \
        test1.sh test2.sh timing1.sh timing2.sh

EXTRA_DIST = zebra1.cfg zebra2.cfg $(check_SCRIPTS)
