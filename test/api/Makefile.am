# $Id: Makefile.am,v 1.11 2003-06-18 11:46:34 adam Exp $

TESTS = t1 t2 t3 t4

testclient_SOURCES = testclient.c

noinst_PROGRAMS = testclient

EXTRA_PROGRAMS = t1 t2 t3 t4

EXTRA_DIST=t1.cfg t2.cfg

t1_SOURCES = t1.c
t2_SOURCES = t2.c
t3_SOURCES = t3.c
t4_SOURCES = t4.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../../index/libzebra.a $(YAZLIB) $(TCL_LIB)
