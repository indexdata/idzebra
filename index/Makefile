# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.1 1995-08-31 14:50:24 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG1=index
DEFS=$(INCLUDE)
O = main.o
CPP=cc -E

all: $(TPROG1)

$(TPROG1): $(O) ../lib/dict.a ../lib/bfile.a ../lib/util.a
	$(CC) $(CFLAGS) -o $(TPROG1) $(O) ../lib/dict.a ../lib/bfile.a ../lib/util.a

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) core mon.out gmon.out errlist

$(O): index.h

depend: depend2

depend1:
	mv Makefile Makefile.tmp
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) $(INCLUDE) -M *.c >>Makefile
	-rm Makefile.tmp

depend2:
	$(CPP) $(INCLUDE) -M *.c >.depend	

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#Depend --- DOT NOT DELETE THIS LINE
