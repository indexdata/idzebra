# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.5 1995-09-06 10:35:44 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib
YAZ=../../yaz
INCLUDE=-I../include -I$(YAZ)/include
#CFLAGS=-g -Wall -pedantic -ansi
DEFS=$(INCLUDE)
LIB=../lib/rset.a
PROG=
PO=rset.o rstemp.o rsisam.o rsnull.o
CPP=cc -E

all: $(LIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	$(RANLIB) $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] core mon.out gmon.out errlist

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
