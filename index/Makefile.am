## $Id: Makefile.am,v 1.37 2006-05-01 08:27:20 adam Exp $

noinst_PROGRAMS = apitest kdump zebrasrv2 zebraidx2

lib_LTLIBRARIES = libidzebra-api.la libidzebra-all.la

libidzebra_all_la_SOURCES = 
libidzebra_api_la_SOURCES = dir.c dirs.c trav.c kinput.c kcompare.c \
 attribute.c symtab.c recindex.c recstat.c \
 zebraapi.c api_swig.c \
 zinfo.c invstat.c sortidx.c compact.c zsets.c zrpn.c \
 rank1.c trunc.c retrieve.c extract.c rankstatic.c \
 index.h recindex.h recindxp.h reckeys.c reckeys.h \
 zinfo.h zserver.h zvrank.c limit.c kcontrol.c orddict.c orddict.h

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

