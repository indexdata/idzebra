# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.1 1994-11-03 14:13:29 quinn Exp $

SHELL=/bin/sh
INCLUDE=-I../include
CFLAGS=-g -Wall -pedantic
DEFS=$(INCLUDE)
LIB=../lib/rset.a
PROG=
PO=
CPP=cc -E

all: $(PROG) $(LIB)

$(PROG): $(PROG).o
	$(CC) -o $(PROG) $(INCLUDE) $(CFLAGS) $(PROG).o $(LIB)


$(LIB): $(PO)
	ar aq $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] core mon.out gmon.out errlist

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
