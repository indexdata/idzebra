# $Id: Makefile.am,v 1.13 2004-08-13 13:16:52 adam Exp $

noinst_PROGRAMS = testclient
testclient_SOURCES = testclient.c

check_PROGRAMS = t1 t2 t3 t4 t5 t6
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebra6.cfg

t1_SOURCES = t1.c
t2_SOURCES = t2.c
t3_SOURCES = t3.c
t4_SOURCES = t4.c
t5_SOURCES = t5.c
t6_SOURCES = t6.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../../index/libzebra.a $(YAZLIB) $(TCL_LIB)

