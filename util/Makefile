# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.16 1995-01-24 16:01:50 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG=opt-test
CFLAGS=-g -Wall -pedantic -ansi
DEFS=$(INCLUDE)
LIB=../lib/util.a
PO = options.o xmalloc.o log.o res.o alexpath.o common.o
CPP=cc -E

all: $(LIB)

alll: opt-test res-test all

opt-test: opt-test.o $(LIB) 
	$(CC) -o opt-test opt-test.o $(LIB)

res-test: res-test.o $(LIB)
	$(CC) -o res-test res-test.o $(LIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] opt-test res-test core mon.out gmon.out errlist

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
