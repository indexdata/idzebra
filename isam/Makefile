SHELL=/bin/sh
INCLUDE=-I../include
CFLAGS=-g -Wall -pedantic
DEFS=$(INCLUDE)
LIB=../lib/isam.a
PO = isam.o isutil.o rootblk.o memory.o
CPP=cc -E

all: $(LIB)

isam-test: isam-test.c $(LIB)
	$(CC) -g -o isam-test -I../include isam-test.c \
	../lib/isam.a ../lib/bfile.a ../lib/util.a

#$(TPROG): $(TPROG).o $(LIB) 
#	$(CC) -o $(TPROG) $(TPROG).o $(LIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG) core mon.out gmon.out errlist isam-test

dep depend:
	mv Makefile Makefile.tmp
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) $(INCLUDE) -M *.c >>Makefile

#Depend --- DOT NOT DELETE THIS LINE
