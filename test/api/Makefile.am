# $Id: Makefile.am,v 1.10 2003-05-21 14:39:22 adam Exp $

TESTS = t1 t2 t3

testclient_SOURCES = testclient.c

noinst_PROGRAMS = testclient

EXTRA_PROGRAMS = t1 t2 t3

EXTRA_DIST=t1.cfg t2.cfg

t1_SOURCES = t1.c
t2_SOURCES = t2.c
t3_SOURCES = t3.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../../index/libzebra.a $(YAZLIB) $(TCL_LIB)
