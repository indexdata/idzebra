## $Id: Makefile.am,v 1.7 2006-06-29 09:10:29 adam Exp $ 

zebralibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)

zebralib_LTLIBRARIES = libidzebra-bfile.la

check_PROGRAMS = tstbfile1

TESTS = $(check_PROGRAMS)

tstbfile1_SOURCES = tstbfile1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

libidzebra_bfile_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_bfile_la_SOURCES = bfile.c mfile.c cfile.c commit.c cfile.h mfile.h

LDADD = libidzebra-bfile.la ../util/libidzebra-util.la $(YAZLALIB)

