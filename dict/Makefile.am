## $Id: Makefile.am,v 1.11 2004-12-08 12:23:08 adam Exp $

lib_LTLIBRARIES = libidzebra-dict.la
noinst_PROGRAMS = dicttest dictext
check_PROGRAMS = scantest

TESTS = $(check_PROGRAMS)

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)

LDADD = libidzebra-dict.la \
 ../bfile/libidzebra-bfile.la \
 ../dfa/libidzebra-dfa.la \
 ../util/libidzebra-util.la \
  $(YAZLALIB)

libidzebra_dict_la_SOURCES = dopen.c dclose.c drdwr.c open.c close.c \
 insert.c lookup.c lookupec.c lookgrep.c delete.c scan.c dcompact.c \
 dict-p.h

dicttest_SOURCES = dicttest.c

dictext_SOURCES = dictext.c

scantest_SOURCES = scantest.c

