# $Id: Makefile.am,v 1.5 2006-07-03 14:27:29 adam Exp $

check_PROGRAMS = t1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg default.idx \
 rec1.xml rec2.xml rec3.xml rec4.xml zebra.cfg my.abs sort.chr

t1_SOURCES = t1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

