## $Id: Makefile.am,v 1.5 2004-08-24 10:37:18 adam Exp $

lib_LTLIBRARIES = libidzebra-isamb.la

check_PROGRAMS = tstisamb

TESTS = $(check_PROGRAMS)

tstisamb_SOURCES = tstisamb.c
tstisamb_LDADD = libisamb.a ../bfile/libbfile.a ../util/libutil.a $(YAZLALIB)

libidzebra_isamb_la_SOURCES = isamb.c

AM_CPPFLAGS=-I$(srcdir)/../include $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB) $(TCL_LIB)
