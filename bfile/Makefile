
SHELL=/bin/sh
INCLUDE=-I../include
TPROG=btest
CFLAGS=-g -Wall
DEFS=$(INCLUDE)
LIB=../lib/bfile.a
PO = open.o close.o rdwr.o
CPP=cc -E

all: $(LIB)

$(TPROG): $(TPROG).o $(LIB) 
	$(CC) -o $(TPROG) $(TPROG).o $(LIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG) core mon.out gmon.out

dep depend:
	$(CPP) $(INCLUDE) -M *.c >.depend

#ifeq (.depend,$(wildcard .depend))
include .depend
#endif
