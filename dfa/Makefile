# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.1 1994-09-26 10:16:52 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG1=agrep
TPROG2=lexer
CFLAGS=-g -Wall -pedantic
DEFS=$(INCLUDE) -DYACC -DYYDEBUG=1 -DMEMDEBUG=1
LIB=../lib/dfa.a 
PO = regexp.o imalloc.o states.o set.o bset.o
CPP=cc -E

all: $(LIB)

$(TPROG1): $(TPROG1).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) ../lib/util.a

$(TPROG2): $(TPROG2).o readfile.o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG2) $(TPROG2).o readfile.o \
	        $(LIB) ../lib/util.a

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
	rm -f *.[oa] $(TPROG1) $(TPROG2) core mon.out gmon.out errlist y.tab.c

depend: depend2

depend1:
	mv Makefile Makefile.tmp
	$(YACC) $(YFLAGS) regexp.y
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) -M $(DEFS) *.c |sed 's/y\.tab\.o/regexp.o/g' >>Makefile
	-rm Makefile.tmp

depend2:
	$(YACC) $(YFLAGS) regexp.y
	$(CPP) -M $(DEFS) *.c |sed 's/y\.tab\.o/regexp.o/g' >.depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#Depend --- DOT NOT DELETE THIS LINE
