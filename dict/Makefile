# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.2 1994-08-17 13:32:18 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG=dicttest
CFLAGS=-g -Wall
DEFS=$(INCLUDE)
LIB=../lib/dict.a 
PO = dopen.o dclose.o drdwr.o open.o close.o insert.o lookup.o 
CPP=cc -E

all: $(LIB)

$(TPROG): $(TPROG).o $(LIB) 
	$(CC) -o $(TPROG) $(TPROG).o $(LIB) ../lib/bfile.a ../lib/util.a

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG) core mon.out gmon.out

dep depend:
	$(CPP) $(INCLUDE) -M *.c >.depend

#ifeq (.depend,$(wildcard .depend))
include .depend
#endif
