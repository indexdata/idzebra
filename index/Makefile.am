## $Id: Makefile.am,v 1.1 2000-04-05 09:59:42 adam Exp $

if ISZMBOL
bin_PROGRAMS = zmbolidx zmbolsrv apitest
zmbolidx_SOURCES = main.c dir.c dirs.c trav.c extract.c kinput.c kcompare.c \
 symtab.c recindex.c recstat.c lockutil.c lockidx.c \
 zinfo.c invstat.c sortidx.c compact.c \
 index.h recindex.h recindxp.h zebraapi.h zinfo.h zserver.h

zmbolsrv_SOURCES = zserver.c kcompare.c zrpn.c zsets.c attribute.c recindex.c \
 lockutil.c locksrv.c zinfo.c trunc.c sortidx.c rank1.c zebraapi.c \
 retrieve.c kinput.c \
 index.h recindex.h recindxp.h zebraapi.h zinfo.h zserver.h
else
bin_PROGRAMS = zebraidx zebrasrv
zebraidx_SOURCES = main.c dir.c dirs.c trav.c extract.c kinput.c kcompare.c \
 symtab.c recindex.c recstat.c lockutil.c lockidx.c \
 zinfo.c invstat.c sortidx.c compact.c \
 index.h recindex.h recindxp.h zebraapi.h zinfo.h zserver.h

zebrasrv_SOURCES = zserver.c kcompare.c zrpn.c zsets.c attribute.c recindex.c \
 lockutil.c locksrv.c zinfo.c trunc.c sortidx.c rank1.c zebraapi.c \
 retrieve.c kinput.c \
 index.h recindex.h recindxp.h zebraapi.h zinfo.h zserver.h
endif

apitest_SOURCES = apitest.c kcompare.c zrpn.c zsets.c attribute.c recindex.c \
 lockutil.c locksrv.c zinfo.c trunc.c sortidx.c rank1.c zebraapi.c \
 retrieve.c kinput.c \
 index.h recindex.h recindxp.h zebraapi.h zinfo.h zserver.h

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = @SUBLIBS@ @YAZLIB@
