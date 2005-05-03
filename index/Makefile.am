## $Id: Makefile.am,v 1.31 2005-05-03 09:11:34 adam Exp $

noinst_PROGRAMS = apitest kdump

lib_LTLIBRARIES = libidzebra-api.la

libidzebra_api_la_SOURCES = dir.c dirs.c trav.c kinput.c kcompare.c \
 attribute.c symtab.c recindex.c recstat.c lockutil.c  \
 zebraapi.c zinfo.c invstat.c sortidx.c compact.c zsets.c zrpn.c \
 rank1.c trunc.c retrieve.c extract.c \
 index.h recindex.h recindxp.h \
 zinfo.h zserver.h zvrank.c limit.c kcontrol.c

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

