# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.8 1995-09-04 12:33:25 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib
YAZ=../../yaz
YAZLIB=$(YAZ)/lib/libyaz.a
INCLUDE=-I../include -I$(YAZ)/include
TPROG1=agrep
TPROG2=lexer
TPROG3=grepper
#CFLAGS=-g -Wall -pedantic -ansi
DEFS=$(INCLUDE) -DYACC -DYYDEBUG=1 -DMEMDEBUG=1
LIB=../lib/dfa.a 
PO = dfa.o imalloc.o states.o set.o bset.o
CPP=cc -E
YACC=yacc

all: $(LIB)

alll: $(LIB) $(TPROG1) $(TPROG2) $(TPROG3)

$(TPROG1): $(TPROG1).o $(LIB) ../lib/alexutil.a $(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) \
		../lib/alexutil.a $(YAZLIB)

$(TPROG2): $(TPROG2).o readfile.o $(LIB) ../lib/alexutil.a $(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG2) $(TPROG2).o readfile.o $(LIB) \
		../lib/alexutil.a $(YAZLIB)

$(TPROG3): $(TPROG3).o $(LIB) ../lib/alexutil.a $(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG3) $(TPROG3).o $(LIB) $(YAZLIB) \
		../lib/alexutil.a $(YAZLIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	$(RANLIB) $(LIB)

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
	$(YACC) $(YFLAGS) regexp.y
	mv y.tab.c regexp.c
	sed '/^#Depend/q' <Makefile >Makefile.tmp
	$(CPP) -M $(DEFS) *.c |sed 's/regexp\.c/regexp.y/g' >>Makefile.tmp
	mv -f Makefile.tmp Makefile
	rm -f regexp.c

depend2:
	$(YACC) $(YFLAGS) regexp.y
	mv y.tab.c regexp.c
	$(CPP) -M $(DEFS) *.c |sed 's/regexp\.c/regexp.y/g' >.depend
	rm -f regexp.c

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#Depend --- DOT NOT DELETE THIS LINE
