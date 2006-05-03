## $Id: Makefile.am,v 1.38 2006-05-03 09:31:25 marc Exp $

noinst_PROGRAMS = apitest kdump zebrasrv2 zebraidx2

lib_LTLIBRARIES = libidzebra-api.la libidzebra-all.la

libidzebra_all_la_SOURCES = 
libidzebra_api_la_SOURCES = \
  api_swig.c attribute.c \
  compact.c \
  dir.c dirs.c \
  extract.c \
  index.h invstat.c \
  kinput.c kcompare.c  kcontrol.c \
  limit.c \
  orddict.c orddict.h \
  rank.h rank1.c ranksimilarity.c rankstatic.c \
  recindex.c  recindex.h recindxp.h reckeys.c reckeys.h recstat.c retrieve.c \
  sortidx.c symtab.c \
  trav.c trunc.c \
  zebraapi.c zinfo.c zinfo.h zserver.h zsets.c zrpn.c \
  zvrank.c

bin_PROGRAMS = zebraidx zebrasrv zebrash

zebralibs = \
 libidzebra-api.la \
 ../rset/libidzebra-rset.la \
 ../recctrl/libidzebra-recctrl.la \
 ../dict/libidzebra-dict.la  \
 ../isams/libidzebra-isams.la \
 ../isamc/libidzebra-isamc.la \
 ../isamb/libidzebra-isamb.la \
 ../data1/libidzebra-data1.la \
 ../bfile/libidzebra-bfile.la \
 ../dfa/libidzebra-dfa.la \
 ../util/libidzebra-util.la


zebraidx_SOURCES = main.c
zebrasrv_SOURCES = zserver.c
zebrash_SOURCES = zebrash.c
apitest_SOURCES = apitest.c
kdump_SOURCES = kdump.c

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) \
  -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"

zebrash_LDADD= $(zebralibs) $(YAZLALIB) $(READLINE_LIBS)

LDADD = $(zebralibs) $(YAZLALIB)

## Try make zebraidx zebrasrv versions with a combined lib
libidzebra_all_la_LIBADD = $(zebralibs) $(YAZLALIB)

zebraidx2_SOURCES = main.c
zebraidx2_LDADD= libidzebra-all.la $(YAZLALIB) 

zebrasrv2_SOURCES = zserver.c
zebrasrv2_LDADD= libidzebra-all.la $(YAZLALIB) 

