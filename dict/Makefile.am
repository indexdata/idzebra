## $Id: Makefile.am,v 1.10 2004-12-07 14:57:08 adam Exp $

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

libidzebra_dict_la_SOURCES = scan.c dopen.c dclose.c drdwr.c open.c \
 close.c insert.c lookup.c lookupec.c lookgrep.c delete.c dcompact.c

dicttest_SOURCES = dicttest.c

dictext_SOURCES = dictext.c

scantest_SOURCES = scantest.c

