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
bfile.o : bfile.c ../include/util.h ../include/log.h ../include/options.h ../include/xmalloc.h \
  /usr/include/sys/types.h /usr/include/linux/types.h ../include/res.h ../include/alexpath.h \
  ../include/common.h ../include/bfile.h ../include/mfile.h /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/fcntl.h /usr/include/linux/fcntl.h /usr/include/unistd.h /usr/include/posix_opt.h \
  /usr/include/gnu/types.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h \
  /usr/include/stdlib.h /usr/include/errno.h /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h 
mfile.o : mfile.c /usr/include/sys/types.h /usr/include/linux/types.h /usr/include/fcntl.h \
  /usr/include/features.h /usr/include/sys/cdefs.h /usr/include/linux/fcntl.h \
  /usr/include/unistd.h /usr/include/posix_opt.h /usr/include/gnu/types.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h \
  /usr/include/dirent.h /usr/include/linux/limits.h /usr/include/linux/dirent.h \
  /usr/include/posix1_lim.h /usr/include/stdlib.h /usr/include/errno.h /usr/include/linux/errno.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h /usr/include/alloca.h /usr/include/assert.h \
  ../include/util.h ../include/log.h ../include/options.h ../include/xmalloc.h \
  ../include/res.h ../include/alexpath.h ../include/common.h ../include/mfile.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h 
