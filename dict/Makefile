# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.19 1995-12-06 14:43:01 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=-lyaz
YAZINC=

INCLUDE=-I../include $(YAZINC)
TPROG1=dicttest
TPROG2=dictext
#CFLAGS=-g -Wall -pedantic -ansi
DEFS=$(INCLUDE)
LIB=../lib/dict.a 
PO = scan.o dopen.o dclose.o drdwr.o open.o close.o insert.o lookup.o \
 lookupec.o lookgrep.o delete.o
CPP=$(CC) -E

all: $(LIB)

alll: $(LIB) $(TPROG1) $(TPROG2)

$(TPROG1): $(TPROG1).o $(LIB) ../lib/bfile.a ../lib/alexutil.a ../lib/dfa.a
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) \
		../lib/bfile.a ../lib/dfa.a ../lib/alexutil.a $(YAZLIB)

$(TPROG2): $(TPROG2).o $(LIB) ../lib/alexutil.a $(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG2) $(TPROG2).o ../lib/alexutil.a $(YAZLIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	$(RANLIB) $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) core mon.out gmon.out errlist

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
