## $Id: Makefile.am,v 1.6 2006-06-26 15:11:21 marc Exp $ Copyright (C) 1994-1998, Index Data ApS
lib_LTLIBRARIES = libidzebra-bfile.la

check_PROGRAMS = tstbfile1

TESTS = $(check_PROGRAMS)

tstbfile1_SOURCES = tstbfile1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

libidzebra_bfile_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_bfile_la_SOURCES = bfile.c mfile.c cfile.c commit.c cfile.h mfile.h

LDADD = libidzebra-bfile.la ../util/libidzebra-util.la $(YAZLALIB)

