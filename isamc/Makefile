# Copyright (C) 1996, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.2 1996-11-01 08:59:11 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=../../yaz/lib/libyaz.a
YAZINC=-I../../yaz/include

INCLUDE=-I../include $(YAZINC)
TPROG1=isamctst
DEFS=$(INCLUDE)
LIB=../lib/isamc.a 
PO = isamc.o merge.o
CPP=$(CC) -E

all: $(LIB)

alll: $(LIB) 

$(TPROG1): $(TPROG1).o $(LIB) ../lib/bfile.a ../lib/zebrautl.a
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) \
		../lib/bfile.a ../lib/zebrautl.a $(YAZLIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	$(RANLIB) $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG1) core mon.out gmon.out errlist

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
