# Copyright (C) 1995-1996, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.1 1996-10-11 10:57:22 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib

YAZLIB=../../yaz/lib/libyaz.a
YAZINC=-I../../yaz/include

INCLUDE=-I../include $(YAZINC)
DEFS=$(INCLUDE)
LIB=../lib/recctrl.a
PROG=
PO=recctrl.o recgrs.o regxread.o sgmlread.o rectext.o
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
