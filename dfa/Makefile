# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.6 1995-01-24 16:00:20 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG1=agrep
TPROG2=lexer
TPROG3=grepper
CFLAGS=-g -Wall -pedantic -ansi
DEFS=$(INCLUDE) -DYACC -DYYDEBUG=1 -DMEMDEBUG=1
LIB=../lib/dfa.a 
PO = dfa.o imalloc.o states.o set.o bset.o
CPP=cc -E
YACC=yacc

all: $(LIB)

alll: $(LIB) $(TPROG1) $(TPROG2) $(TPROG3)

$(TPROG1): $(TPROG1).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) ../lib/util.a

$(TPROG2): $(TPROG2).o readfile.o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG2) $(TPROG2).o readfile.o $(LIB) ../lib/util.a

$(TPROG3): $(TPROG3).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG3) $(TPROG3).o $(LIB) ../lib/util.a

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

.y.o:
	$(YACC) $(YFLAGS) $<
	$(CC) -c $(DEFS) $(CFLAGS) y.tab.c
	mv y.tab.o $*.o

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) $(TPROG3) core mon.out gmon.out errlist y.tab*

depend: depend2

depend1:
	mv Makefile Makefile.tmp
	$(YACC) $(YFLAGS) regexp.y
	mv y.tab.c regexp.c
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) -M $(DEFS) *.c |sed 's/regexp\.c/regexp.y/g' >>Makefile
	rm -f Makefile.tmp regexp.c

depend2:
	$(YACC) $(YFLAGS) regexp.y
	mv y.tab.c regexp.c
	$(CPP) -M $(DEFS) *.c |sed 's/regexp\.c/regexp.y/g' >.depend
	rm -f regexp.c

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#Depend --- DOT NOT DELETE THIS LINE
