# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.6 1994-09-12 08:06:41 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG=dicttest
CFLAGS=-O -Wall -g -pedantic
DEFS=$(INCLUDE)
LIB=../lib/dict.a 
PO = dopen.o dclose.o drdwr.o open.o close.o insert.o lookup.o 
CPP=cc -E

all: $(LIB)

$(TPROG): $(TPROG).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG) $(TPROG).o $(LIB) ../lib/bfile.a ../lib/util.a

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG) core mon.out gmon.out errlist

dep depend:
	$(CPP) $(INCLUDE) -M *.c >.depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
