## $Id: Makefile.am,v 1.7 2004-09-30 08:16:49 adam Exp $

lib_LTLIBRARIES = libidzebra-isamc.la

libidzebra_isamc_la_SOURCES = isamc.c merge.c isamc-p.h

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../index $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB)
