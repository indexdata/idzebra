# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.7 1994-09-16 15:39:10 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG1=dicttest
TPROG2=dictext
CFLAGS=-O -Wall -g -pedantic
DEFS=$(INCLUDE)
LIB=../lib/dict.a 
PO = dopen.o dclose.o drdwr.o open.o close.o insert.o lookup.o 
CPP=cc -E

all: $(LIB)

$(TPROG1): $(TPROG1).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) ../lib/bfile.a ../lib/util.a

$(TPROG2): $(TPROG2).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG2) $(TPROG2).o ../lib/util.a


$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) core mon.out gmon.out errlist

dep depend:
	$(CPP) $(INCLUDE) -M *.c >.depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
