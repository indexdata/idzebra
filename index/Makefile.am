## $Id: Makefile.am,v 1.4 2000-10-17 12:37:09 adam Exp $

noinst_PROGRAMS = apitest

if ISZMBOL
bin_PROGRAMS = zmbolidx zmbolsrv 
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

INCLUDES = -I$(srcdir)/../include $(YAZINC) $(TCL_INCLUDE)

if ISZMBOL
zmbol_libs = ../isam/libisam.a ../isamc/libisamc.a ../isamb/libisamb.a
else
zmbol_libs =
endif

LDADD = \
 ../rset/librset.a ../dict/libdict.a  ../isams/libisams.a \
 $(zmbol_libs) \
 ../recctrl/librecctrl.a ../bfile/libbfile.a ../dfa/libdfa.a \
 ../util/libutil.a \
 $(YAZLIB) $(TCL_LIB)
