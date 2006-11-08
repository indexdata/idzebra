## $Id: Makefile.am,v 1.11 2006-11-08 12:59:27 adam Exp $ 

noinst_LTLIBRARIES = libidzebra-bfile.la

check_PROGRAMS = tstbfile1 tstbfile2

TESTS = $(check_PROGRAMS)

tstbfile1_SOURCES = tstbfile1.c
tstbfile2_SOURCES = tstbfile2.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

libidzebra_bfile_la_SOURCES = bfile.c mfile.c cfile.c commit.c cfile.h mfile.h

LDADD = libidzebra-bfile.la ../util/libidzebra-util.la $(YAZLALIB)

