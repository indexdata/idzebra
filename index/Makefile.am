## $Id: Makefile.am,v 1.11 2002-04-05 08:46:26 adam Exp $

noinst_PROGRAMS = apitest kdump

noinst_LIBRARIES = libzebra.a

libzebra_a_SOURCES = dir.c dirs.c trav.c kinput.c kcompare.c \
 attribute.c symtab.c recindex.c recstat.c lockutil.c  \
 zebraapi.c zinfo.c invstat.c sortidx.c compact.c zsets.c zrpn.c \
 rank1.c trunc.c retrieve.c extract.c index.h recindex.h recindxp.h \
 zebraapi.h zinfo.h zserver.h

libzebra_a_LIBADD = \
 ../rset/librset.a ../dict/libdict.a  ../isams/libisams.a \
 ../isam/libisam.a ../isamc/libisamc.a ../isamb/libisamb.a \
 ../recctrl/librecctrl.a ../bfile/libbfile.a ../dfa/libdfa.a \
 ../util/libutil.a

if ISZMBOL
bin_PROGRAMS = zmbolidx zmbolsrv 
else
bin_PROGRAMS = zebraidx zebrasrv
endif

zmbolidx_SOURCES = main.c
zmbolsrv_SOURCES = zserver.c
zebraidx_SOURCES = main.c
zebrasrv_SOURCES = zserver.c
apitest_SOURCES = apitest.c
kdump_SOURCES=kdump.c kcompare.c

CFLAGS = -I$(srcdir)/../include $(YAZINC) $(TCL_INCLUDE)

LDADD = libzebra.a $(YAZLIB) $(TCL_LIB)

libzebra.a: $(libzebra_a_OBJECTS) $(libzebra_a_DEPENDENCIES)
	-mkdir atmp
	-rm -f libzebra.a
	-rm -f atmp/*.o
	for i in $(libzebra_a_LIBADD); do cd atmp; rm -f __*; ar x ../$$i; cd ..; done
	ar cru libzebra.a $(libzebra_a_OBJECTS) atmp/*.o
	$(RANLIB) libzebra.a
