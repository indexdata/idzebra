## $Id: Makefile.am,v 1.7 2004-09-30 08:16:48 adam Exp $

lib_LTLIBRARIES = libidzebra-isamb.la

check_PROGRAMS = tstisamb

TESTS = $(check_PROGRAMS)

tstisamb_SOURCES = tstisamb.c
tstisamb_LDADD = libidzebra-isamb.la \
 ../bfile/libidzebra-bfile.la \
 ../util/libidzebra-util.la $(YAZLALIB)

libidzebra_isamb_la_SOURCES = isamb.c

AM_CPPFLAGS=-I$(srcdir)/../include $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB)
