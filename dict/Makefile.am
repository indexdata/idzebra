## $Id: Makefile.am,v 1.13 2006-06-26 15:11:28 marc Exp $

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

libidzebra_dict_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_dict_la_SOURCES = dopen.c dclose.c drdwr.c open.c close.c \
 insert.c lookup.c lookupec.c lookgrep.c delete.c scan.c dcompact.c \
 dict-p.h

dicttest_SOURCES = dicttest.c

dictext_SOURCES = dictext.c

scantest_SOURCES = scantest.c


clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

