# Copyright (C) 1994-1996, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.28 1998-06-22 11:36:51 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=../../yaz/lib/libyaz.a
YAZINC=-I../../yaz/include

INCLUDE=-I../include $(YAZINC)
TPROG=opt-test
DEFS=$(INCLUDE) -DUSE_CRYPT=0
CPP=$(CC) -E
LIB=../lib/zebrautl.a
PO = res.o charmap.o zebramap.o passwddb.o

all: $(LIB)

alll: res-test all passtest

res-test: res-test.o $(LIB) 
	$(CC) -o res-test res-test.o $(LIB) $(YAZLIB)

passtest: passtest.o $(LIB) 
	$(CC) -o passtest passtest.o $(LIB) $(YAZLIB) -lcrypt

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	$(RANLIB) $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] res-test core mon.out gmon.out errlist

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
