# $Id: Makefile.am,v 1.6 2006-07-03 14:27:18 adam Exp $

check_PROGRAMS = tstcodec

TESTS = $(check_PROGRAMS)

tstcodec_SOURCES = tstcodec.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../../$(main_zebralib) $(YAZLALIB)
