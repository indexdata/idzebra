## $Id: Makefile.am,v 1.9 2006-07-03 14:27:01 adam Exp $ 

noinst_LTLIBRARIES = libidzebra-bfile.la

check_PROGRAMS = tstbfile1

TESTS = $(check_PROGRAMS)

tstbfile1_SOURCES = tstbfile1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

libidzebra_bfile_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_bfile_la_SOURCES = bfile.c mfile.c cfile.c commit.c cfile.h mfile.h

LDADD = libidzebra-bfile.la ../util/libidzebra-util.la $(YAZLALIB)

