# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.4 1994-08-17 15:34:22 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG=opt-test
CFLAGS=-g -Wall
DEFS=$(INCLUDE)
LIB=../lib/util.a
PO = options.o xmalloc.o log.o res.o
CPP=cc -E

all: $(LIB)

$(TPROG): $(TPROG).o $(LIB) 
	$(CC) -o $(TPROG) $(TPROG).o $(LIB)

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
