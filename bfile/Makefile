# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.13 1995-10-30 14:10:47 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib
YAZ=../../yaz
YAZLIB=$(YAZ)/lib/libyaz.a
INCLUDE=-I../include -I$(YAZ)/include
#CFLAGS=-g -Wall -pedantic -ansi
DEFS=$(INCLUDE)
TPROG=btest
CFLAGS=-g -Wall -pedantic -ansi
DEFS=$(INCLUDE)
LIB=../lib/bfile.a
PO = bfile.o mfile.o
CPP=$(CC) -E

all: $(LIB)

$(TPROG): $(TPROG).o $(LIB) $(YAZLIB)
	$(CC) -o $(TPROG) $(TPROG).o $(LIB) $(YAZLIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	$(RANLIB) $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG) core mon.out gmon.out errlist


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
