SHELL=/bin/sh
INCLUDE=-I../include
CFLAGS=-g -Wall -pedantic
DEFS=$(INCLUDE)
LIB=../lib/isam.a
PO = isutil.o isam.o
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
	$(CPP) $(INCLUDE) -M *.c >.depend

#ifeq (.depend,$(wildcard .depend))
include .depend
#endif
