## $Id: Makefile.am,v 1.7 2004-08-24 10:37:18 adam Exp $

lib_LTLIBRARIES = libidzebra-dict.la
noinst_PROGRAMS = dicttest dictext

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)

LDADD = libidzebra-dict.la \
 ../util/libidzebra-util.la \
 ../bfile/libidzebra-bfile.la \
 ../dfa/libidzebra-dfa.la \
  $(TCL_LIB) $(YAZLALIB)

libidzebra_dict_la_SOURCES = scan.c dopen.c dclose.c drdwr.c open.c close.c insert.c \
 lookup.c lookupec.c lookgrep.c delete.c dcompact.c

dicttest_SOURCES = dicttest.c

dictext_SOURCES = dictext.c

