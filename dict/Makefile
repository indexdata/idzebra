# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.12 1994-10-03 17:23:01 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG1=dicttest
TPROG2=dictext
CFLAGS=-g -Wall -pedantic 
DEFS=$(INCLUDE)
LIB=../lib/dict.a 
PO = dopen.o dclose.o drdwr.o open.o close.o insert.o lookup.o \
 lookupec.o lookgrep.o
CPP=cc -E

all: $(LIB)

alll: $(LIB) $(TPROG1) $(TPROG2)

$(TPROG1): $(TPROG1).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) ../lib/bfile.a ../lib/dfa.a ../lib/util.a 

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
