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
	mv Makefile Makefile.tmp
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) $(INCLUDE) -M *.c >>Makefile

#Depend --- DOT NOT DELETE THIS LINE
isam.o : isam.c /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h /usr/include/alloca.h /usr/include/string.h \
  /usr/include/ctype.h ../include/util.h ../include/log.h ../include/options.h \
  ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h ../include/res.h \
  ../include/alexpath.h ../include/common.h isutil.h ../include/bfile.h ../include/mfile.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h ../include/isam.h 
isutil.o : isutil.c /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stdarg.h \
  ../include/isam.h ../include/bfile.h ../include/util.h ../include/log.h ../include/options.h \
  ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h ../include/res.h \
  ../include/alexpath.h ../include/common.h ../include/mfile.h /usr/include/stdio.h \
  /usr/include/libio.h /usr/include/_G_config.h isutil.h 
