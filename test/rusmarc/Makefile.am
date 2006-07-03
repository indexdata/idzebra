# $Id: Makefile.am,v 1.7 2006-07-03 14:27:27 adam Exp $

check_PROGRAMS = t1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg test.sh 

dist-hook:
	mkdir -p $(distdir)/records
	mkdir -p $(distdir)/tab
	-cp -p $(srcdir)/records/* $(distdir)/records
	-cp -p $(srcdir)/tab/* $(distdir)/tab

t1_SOURCES = t1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
