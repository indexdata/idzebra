# Copyright (C) 1994-1996, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.12 1996-05-22 08:26:56 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=../../yaz/lib/libyaz.a
YAZINC=-I../../yaz/include

INCLUDE=-I../include $(YAZINC)
DEFS=$(INCLUDE)
LIB=../lib/rset.a
PROG=
PO=rset.o rstemp.o rsisam.o rsnull.o rsbool.o rssbool.o rsrel.o 
CPP=$(CC) -E

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
