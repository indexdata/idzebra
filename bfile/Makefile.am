## $Id: Makefile.am,v 1.5 2005-03-30 09:25:23 adam Exp $ Copyright (C) 1994-1998, Index Data ApS
lib_LTLIBRARIES = libidzebra-bfile.la

check_PROGRAMS = tstbfile1

TESTS = $(check_PROGRAMS)

tstbfile1_SOURCES = tstbfile1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

libidzebra_bfile_la_SOURCES = bfile.c mfile.c cfile.c commit.c cfile.h mfile.h

LDADD = libidzebra-bfile.la ../util/libidzebra-util.la $(YAZLALIB)

