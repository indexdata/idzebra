# Copyright (C) 1994-1996, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.24 1996-05-31 09:07:17 quinn Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=-lyaz
YAZINC=-I../../yaz/include

INCLUDE=-I../include $(YAZINC)
TPROG=opt-test
DEFS=$(INCLUDE)
CPP=$(CC) -E
LIB=../lib/alexutil.a
PO = res.o alexpath.o common.o charmap.o

all: $(LIB)

alll: res-test all

res-test: res-test.o $(LIB) 
	$(CC) -o res-test res-test.o $(LIB) $(YAZLIB)

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
