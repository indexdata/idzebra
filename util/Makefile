# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.13 1994-09-20 09:03:03 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG=opt-test
CFLAGS=-g -Wall -pedantic
DEFS=$(INCLUDE)
LIB=../lib/util.a
PO = options.o xmalloc.o log.o res.o alexpath.o common.o
CPP=cc -E

all: $(LIB)

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

dep depend:
	mv Makefile Makefile.tmp
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) $(INCLUDE) -M *.c >>Makefile

#Depend --- DOT NOT DELETE THIS LINE
