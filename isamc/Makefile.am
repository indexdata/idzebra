## $Id: Makefile.am,v 1.11 2006-07-03 14:27:11 adam Exp $

noinst_LTLIBRARIES = libidzebra-isamc.la

libidzebra_isamc_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_isamc_la_SOURCES = isamc.c merge.c isamc-p.h

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../index $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB)
