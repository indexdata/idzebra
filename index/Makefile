# Copyright (C) 1995, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.11 1995-09-27 12:22:27 adam Exp $

SHELL=/bin/sh
RANLIB=ranlib
YAZ=../../yaz
YAZLIB=$(YAZ)/lib/libyaz.a
OSILIB=../../xtimosi/src/libmosi.a $(YAZ)/lib/librfc.a
INCLUDE=-I../include -I$(YAZ)/include
TPROG1=index
TPROG2=kdump
TPROG3=zserver
DEFS=$(INCLUDE)
O1 = main.o dir.o trav.o extract.o kinput.o kcompare.o ksort.o\
 symtab.o text.o recctrl.o
O2 = kdump.o
O3 = zserver.o kcompare.o zrpn.o zsets.o text.o recctrl.o
CPP=cc -E

all: $(TPROG1) $(TPROG2) $(TPROG3)

$(TPROG1): $(O1) ../lib/dict.a \
		../lib/isam.a ../lib/bfile.a ../lib/alexutil.a ../lib/data1.a \
		$(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG1) $(O1) ../lib/dict.a \
		../lib/isam.a ../lib/bfile.a ../lib/alexutil.a ../lib/data1.a \
		$(YAZLIB) $(OSILIB)

$(TPROG2): $(O2) $(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG2) $(O2) $(YAZLIB)

$(TPROG3): $(O3) \
		../lib/rset.a ../lib/dict.a ../lib/isam.a ../lib/bfile.a \
		../lib/dfa.a ../lib/alexutil.a ../lib/data1.a \
		$(YAZLIB)
	$(CC) $(CFLAGS) -o $(TPROG3) $(O3) \
		../lib/rset.a ../lib/dict.a ../lib/isam.a ../lib/bfile.a \
		../lib/dfa.a ../lib/alexutil.a ../lib/data1.a \
		$(YAZLIB) $(OSILIB) -lm

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) $(TPROG3)
	rm -f core mon.out gmon.out errlist

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
