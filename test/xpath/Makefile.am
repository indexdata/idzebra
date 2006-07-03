# $Id: Makefile.am,v 1.13 2006-07-03 14:27:34 adam Exp $

check_PROGRAMS = xpath1 xpath2 xpath3 xpath4 xpath5 xpath6

TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg rec1.xml rec2.xml assembled.abs

xpath1_SOURCES = xpath1.c
xpath2_SOURCES = xpath2.c
xpath3_SOURCES = xpath3.c
xpath4_SOURCES = xpath4.c
xpath5_SOURCES = xpath5.c
xpath6_SOURCES = xpath6.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
