## $Id: Makefile.am,v 1.9 2004-09-30 08:16:48 adam Exp $

lib_LTLIBRARIES = libidzebra-dict.la
noinst_PROGRAMS = dicttest dictext

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)

LDADD = libidzebra-dict.la \
 ../bfile/libidzebra-bfile.la \
 ../dfa/libidzebra-dfa.la \
 ../util/libidzebra-util.la \
  $(YAZLALIB)

libidzebra_dict_la_SOURCES = scan.c dopen.c dclose.c drdwr.c open.c close.c insert.c \
 lookup.c lookupec.c lookgrep.c delete.c dcompact.c

dicttest_SOURCES = dicttest.c

dictext_SOURCES = dictext.c

