# Copyright (C) 1995-1996, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.43 1997-09-04 13:59:28 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=../../yaz/lib/libyaz.a
#YAZLIB=-lyaz
YAZINC=-I../../yaz/include
#OSILIB=../../xtimosi/src/libmosi.a ../../yaz/lib/librfc.a
#OSILIB=../../xtimosi/src/libmosi.a -lrfc
#NETLIB=-lnsl -lsocket

INCLUDE=-I../include $(YAZINC)
TPROG1=zebraidx
TPROG2=kdump
TPROG3=zebrasrv
DEFS=$(INCLUDE)
O1 = main.o dir.o dirs.o trav.o extract.o kinput.o kcompare.o \
 symtab.o recindex.o recstat.o lockutil.o lockidx.o \
 zinfo.o invstat.o keychars.o
O2 = kdump.o
O3 = zserver.o kcompare.o zrpn.o zsets.o attribute.o recindex.o \
 zlogs.o lockutil.o locksrv.o zinfo.o keychars.o trunc.o
CPP=$(CC) -E

all: $(TPROG1) $(TPROG2) $(TPROG3)

$(TPROG1): $(O1) ../lib/dict.a ../lib/isam.a ../lib/isamc.a ../lib/recctrl.a \
		../lib/bfile.a ../lib/dfa.a ../lib/zebrautl.a $(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG1) $(O1) \
		../lib/dict.a ../lib/isam.a ../lib/isamc.a ../lib/recctrl.a \
		../lib/bfile.a ../lib/dfa.a ../lib/zebrautl.a \
		$(YAZLIB)

$(TPROG2): $(O2)
	$(CC) $(CFLAGS) -o $(TPROG2) $(O2) ../lib/zebrautl.a $(YAZLIB)

$(TPROG3): $(O3) \
		../lib/rset.a ../lib/dict.a ../lib/isam.a ../lib/recctrl.a \
		../lib/isamc.a ../lib/bfile.a ../lib/dfa.a ../lib/zebrautl.a \
		$(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG3) $(O3) \
		../lib/rset.a ../lib/dict.a ../lib/isam.a ../lib/recctrl.a \
		../lib/isamc.a ../lib/bfile.a ../lib/dfa.a ../lib/zebrautl.a \
		$(YAZLIB) $(OSILIB) $(NETLIB) -lm

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) $(TPROG3)
	rm -f core mon.out gmon.out errlist

depend: depend2

depend1:
	sed '/^#Depend/q' <Makefile >Makefile.tmp
	$(CPP) $(DEFS) -M *.c >>Makefile.tmp
	mv -f Makefile.tmp Makefile

depend2:
	$(CPP) $(DEFS) -M *.c >.depend	

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#Depend --- DOT NOT DELETE THIS LINE
