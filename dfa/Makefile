# Copyright (C) 1994-1996, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.14 1996-05-14 11:33:40 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=../../yaz/lib/libyaz.a
YAZINC=-I../../yaz/include

INCLUDE=-I../include $(YAZINC)
TPROG1=agrep
TPROG2=lexer
TPROG3=grepper
DEFS=$(INCLUDE)
LIB=../lib/dfa.a 
PO = dfa.o imalloc.o states.o set.o bset.o
CPP=$(CC) -E

all: $(LIB)

alll: $(LIB) $(TPROG1) $(TPROG2) $(TPROG3)

$(TPROG1): $(TPROG1).o $(LIB) ../lib/alexutil.a 
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) \
		../lib/alexutil.a $(YAZLIB)

$(TPROG2): $(TPROG2).o readfile.o $(LIB) ../lib/alexutil.a
	$(CC) $(CFLAGS) -o $(TPROG2) $(TPROG2).o readfile.o $(LIB) \
		../lib/alexutil.a $(YAZLIB)

$(TPROG3): $(TPROG3).o $(LIB) ../lib/alexutil.a
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
	$(CPP) $(DEFS) -M *.c >>Makefile.tmp
	mv -f Makefile.tmp Makefile

depend2:
	$(CPP) $(DEFS) -M *.c >.depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#Depend --- DOT NOT DELETE THIS LINE
