
noinst_LTLIBRARIES = libidzebra-dict.la
noinst_PROGRAMS = dicttest dictext
check_PROGRAMS = scantest inserttest

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

inserttest_SOURCES = inserttest.c

dictext_SOURCES = dictext.c

scantest_SOURCES = scantest.c

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

