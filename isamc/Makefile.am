## $Id: Makefile.am,v 1.12 2006-07-05 15:03:49 adam Exp $

noinst_LTLIBRARIES = libidzebra-isamc.la

libidzebra_isamc_la_SOURCES = isamc.c merge.c isamc-p.h

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../index $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB)
