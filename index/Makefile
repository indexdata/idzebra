# Copyright (C) 1995, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.4 1995-09-04 09:10:34 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include -I../../yaz/include
TPROG1=index
TPROG2=kdump
TPROG3=zserver
DEFS=$(INCLUDE)
O1 = main.o dir.o trav.o extract.o kinput.o kcompare.o ksort.o
O2 = kdump.o
O3 = zserver.o kcompare.o zrpn.o
CPP=cc -E

all: $(TPROG1) $(TPROG2) $(TPROG3)

$(TPROG1): $(O1) ../lib/dict.a ../lib/isam.a ../lib/bfile.a ../lib/util.a
	$(CC) $(CFLAGS) -o $(TPROG1) $(O1) ../lib/dict.a ../lib/isam.a ../lib/bfile.a ../lib/util.a

$(TPROG2): $(O2) ../lib/util.a
	$(CC) $(CFLAGS) -o $(TPROG2) $(O2) ../lib/util.a

$(TPROG3): $(O3) ../lib/dict.a ../lib/isam.a ../lib/bfile.a ../lib/rset.a ../lib/util.a
	$(CC) $(CFLAGS) -o $(TPROG3) $(O3) ../lib/rset.a ../lib/dict.a ../lib/isam.a ../lib/bfile.a ../lib/util.a


.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) core mon.out gmon.out errlist

$(O1) $(O2) $(O3): index.h

depend: depend2

depend1:
	mv Makefile Makefile.tmp
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) $(INCLUDE) -M *.c >>Makefile
	-rm Makefile.tmp

depend2:
	$(CPP) $(INCLUDE) -M *.c >.depend	

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#Depend --- DOT NOT DELETE THIS LINE
