# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.18 1996-02-10 12:20:54 quinn Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=-lyaz
YAZINC=

INCLUDE=-I../include $(YAZINC)
#CFLAGS=-g -Wall -pedantic -ansi
DEFS=$(INCLUDE)
LIB=../lib/isam.a
PO = isam.o isutil.o rootblk.o memory.o physical.o
CPP=$(CC) -E

all: $(LIB)

alll: $(LIB) # isam-test issh

test: test.c $(LIB)
	$(CC) -g -o test -I../include test.c \
	../lib/isam.a ../lib/bfile.a ../lib/alexutil.a $(YAZLIB)

isam-test: isam-test.c $(LIB)
	$(CC) -g -o isam-test -I../include isam-test.c \
	../lib/isam.a ../lib/bfile.a ../lib/alexutil.a $(YAZLIB)

issh: issh.c $(LIB)
	$(CC) -g -o issh $(INCLUDE) issh.c \
	../lib/isam.a ../lib/bfile.a ../lib/alexutil.a $(YAZLIB)

#$(TPROG): $(TPROG).o $(LIB) 
#	$(CC) -o $(TPROG) $(TPROG).o $(LIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	$(RANLIB) $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG) core mon.out gmon.out errlist test isam-test issh

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
