## $Id: Makefile.am,v 1.9 2006-06-29 09:10:40 adam Exp $

zebralibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)

zebralib_LTLIBRARIES = libidzebra-isamc.la

libidzebra_isamc_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_isamc_la_SOURCES = isamc.c merge.c isamc-p.h

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../index $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB)
