SHELL=/bin/sh
INCLUDE=-I../include
TPROG=btest
CFLAGS=-g -Wall -pedantic
DEFS=$(INCLUDE)
LIB=../lib/bfile.a
PO = bfile.o mfile.o
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
	rm -f *.[oa] $(TPROG) core mon.out gmon.out errlist

dep depend:
	mv Makefile Makefile.tmp
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) $(INCLUDE) -M *.c >>Makefile

#Depend --- DOT NOT DELETE THIS LINE
