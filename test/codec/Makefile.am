# $Id: Makefile.am,v 1.1 2004-06-09 12:13:03 adam Exp $

TESTS = tstcodec

tstcodec_SOURCES = tstcodec.c

noinst_PROGRAMS = tstcodec

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../../index/libzebra.a $(YAZLIB) $(TCL_LIB)
