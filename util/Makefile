# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.12 1994-09-19 16:34:38 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG=opt-test
CFLAGS=-g -Wall -pedantic
DEFS=$(INCLUDE)
LIB=../lib/util.a
PO = options.o xmalloc.o log.o res.o alexpath.o common.o
CPP=cc -E

all: $(LIB)

opt-test: opt-test.o $(LIB) 
	$(CC) -o opt-test opt-test.o $(LIB)

res-test: res-test.o $(LIB)
	$(CC) -o res-test res-test.o $(LIB)

$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] opt-test res-test core mon.out gmon.out errlist

dep depend:
	mv Makefile Makefile.tmp
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) $(INCLUDE) -M *.c >>Makefile

#Depend --- DOT NOT DELETE THIS LINE
alexpath.o : alexpath.c /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/stdlib.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h \
  /usr/include/errno.h /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h /usr/include/string.h ../include/util.h ../include/log.h \
  ../include/options.h ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h \
  ../include/res.h ../include/alexpath.h ../include/common.h 
common.o : common.c ../include/util.h ../include/log.h ../include/options.h \
  ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h ../include/res.h \
  ../include/alexpath.h ../include/common.h 
log.o : log.c ../include/log.h /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/libio.h /usr/include/_G_config.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stdarg.h \
  /usr/include/errno.h /usr/include/linux/errno.h 
opt-test.o : opt-test.c /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/libio.h /usr/include/_G_config.h ../include/options.h 
options.o : options.c /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h /usr/include/alloca.h ../include/options.h 
res-test.o : res-test.c /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/libio.h /usr/include/_G_config.h ../include/util.h ../include/log.h \
  ../include/options.h ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h \
  ../include/res.h ../include/alexpath.h ../include/common.h 
res.o : res.c /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/stdlib.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h \
  /usr/include/errno.h /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h /usr/include/string.h /usr/include/assert.h /usr/include/unistd.h \
  /usr/include/posix_opt.h /usr/include/gnu/types.h /usr/include/sys/types.h /usr/include/linux/types.h \
  ../include/util.h ../include/log.h ../include/options.h ../include/xmalloc.h \
  ../include/res.h ../include/alexpath.h ../include/common.h 
xmalloc.o : xmalloc.c /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/stdlib.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h \
  /usr/include/errno.h /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h /usr/include/string.h ../include/xmalloc.h /usr/include/sys/types.h \
  /usr/include/linux/types.h 
