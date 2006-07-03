# $Id: Makefile.am,v 1.3 2006-07-03 14:27:20 adam Exp $

check_PROGRAMS = t1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg rec1.xml root.abs brief.est

t1_SOURCES = t1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

