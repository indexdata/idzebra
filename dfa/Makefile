# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.10 1995-09-28 09:18:50 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib
YAZ=../../yaz
YAZLIB=$(YAZ)/lib/libyaz.a
INCLUDE=-I../include -I$(YAZ)/include
TPROG1=agrep
TPROG2=lexer
TPROG3=grepper
DEFS=$(INCLUDE) -DMEMDEBUG=1
LIB=../lib/dfa.a 
PO = dfa.o imalloc.o states.o set.o bset.o
CPP=cc -E

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

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) $(TPROG3) core mon.out gmon.out errlist

depend: depend2

depend1:
	sed '/^#Depend/q' <Makefile >Makefile.tmp
	$(CPP) -M $(DEFS) *.c >>Makefile.tmp
	mv -f Makefile.tmp Makefile

depend2:
	$(CPP) -M $(DEFS) *.c >.depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#Depend --- DOT NOT DELETE THIS LINE
