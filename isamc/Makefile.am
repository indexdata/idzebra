## $Id: Makefile.am,v 1.6 2004-08-24 10:37:18 adam Exp $

lib_LTLIBRARIES = libidzebra-isamc.la

libidzebra_isamc_la_SOURCES = isamc.c merge.c isamc-p.h

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../index $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB) $(TCL_LIB)
