# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.8 1994-09-19 16:34:25 adam Exp $

SHELL=/bin/sh
INCLUDE=-I../include
TPROG1=dicttest
TPROG2=dictext
CFLAGS=-O -Wall -g -pedantic
DEFS=$(INCLUDE)
LIB=../lib/dict.a 
PO = dopen.o dclose.o drdwr.o open.o close.o insert.o lookup.o 
CPP=cc -E

all: $(LIB)

$(TPROG1): $(TPROG1).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG1) $(TPROG1).o $(LIB) ../lib/bfile.a ../lib/util.a

$(TPROG2): $(TPROG2).o $(LIB) 
	$(CC) $(CFLAGS) -o $(TPROG2) $(TPROG2).o ../lib/util.a


$(LIB): $(PO)
	rm -f $(LIB)
	ar qc $(LIB) $(PO)
	ranlib $(LIB)

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

clean:
	rm -f *.[oa] $(TPROG1) $(TPROG2) core mon.out gmon.out errlist

dep depend:
	mv Makefile Makefile.tmp
	sed '/^#Depend/q' <Makefile.tmp >Makefile
	$(CPP) $(INCLUDE) -M *.c >>Makefile

#Depend --- DOT NOT DELETE THIS LINE
close.o : close.c /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h /usr/include/alloca.h /usr/include/string.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h /usr/include/assert.h \
  ../include/dict.h ../include/bfile.h ../include/util.h ../include/log.h ../include/options.h \
  ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h ../include/res.h \
  ../include/alexpath.h ../include/common.h ../include/mfile.h 
dclose.o : dclose.c /usr/include/sys/types.h /usr/include/linux/types.h /usr/include/fcntl.h \
  /usr/include/features.h /usr/include/sys/cdefs.h /usr/include/linux/fcntl.h \
  /usr/include/unistd.h /usr/include/posix_opt.h /usr/include/gnu/types.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h /usr/include/stdlib.h \
  /usr/include/errno.h /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h ../include/dict.h ../include/bfile.h ../include/util.h \
  ../include/log.h ../include/options.h ../include/xmalloc.h ../include/res.h \
  ../include/alexpath.h ../include/common.h ../include/mfile.h 
dictext.o : dictext.c /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/include/stdlib.h /usr/include/errno.h \
  /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/assert.h /usr/include/ctype.h ../include/util.h ../include/log.h \
  ../include/options.h ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h \
  ../include/res.h ../include/alexpath.h ../include/common.h 
dicttest.o : dicttest.c /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h /usr/include/alloca.h /usr/include/string.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h /usr/include/ctype.h \
  ../include/dict.h ../include/bfile.h ../include/util.h ../include/log.h ../include/options.h \
  ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h ../include/res.h \
  ../include/alexpath.h ../include/common.h ../include/mfile.h 
dopen.o : dopen.c /usr/include/sys/types.h /usr/include/linux/types.h /usr/include/fcntl.h \
  /usr/include/features.h /usr/include/sys/cdefs.h /usr/include/linux/fcntl.h \
  /usr/include/unistd.h /usr/include/posix_opt.h /usr/include/gnu/types.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h /usr/include/stdlib.h \
  /usr/include/errno.h /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h ../include/dict.h ../include/bfile.h ../include/util.h \
  ../include/log.h ../include/options.h ../include/xmalloc.h ../include/res.h \
  ../include/alexpath.h ../include/common.h ../include/mfile.h 
drdwr.o : drdwr.c /usr/include/sys/types.h /usr/include/linux/types.h /usr/include/fcntl.h \
  /usr/include/features.h /usr/include/sys/cdefs.h /usr/include/linux/fcntl.h \
  /usr/include/unistd.h /usr/include/posix_opt.h /usr/include/gnu/types.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h \
  /usr/include/string.h /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/stdlib.h /usr/include/errno.h /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h /usr/include/assert.h ../include/dict.h ../include/bfile.h \
  ../include/util.h ../include/log.h ../include/options.h ../include/xmalloc.h \
  ../include/res.h ../include/alexpath.h ../include/common.h ../include/mfile.h 
insert.o : insert.c /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/include/stdlib.h /usr/include/errno.h \
  /usr/include/linux/errno.h /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h \
  /usr/include/alloca.h /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/assert.h ../include/dict.h ../include/bfile.h ../include/util.h \
  ../include/log.h ../include/options.h ../include/xmalloc.h /usr/include/sys/types.h \
  /usr/include/linux/types.h ../include/res.h ../include/alexpath.h ../include/common.h \
  ../include/mfile.h 
lookup.o : lookup.c /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h /usr/include/alloca.h /usr/include/string.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h /usr/include/assert.h \
  ../include/dict.h ../include/bfile.h ../include/util.h ../include/log.h ../include/options.h \
  ../include/xmalloc.h /usr/include/sys/types.h /usr/include/linux/types.h ../include/res.h \
  ../include/alexpath.h ../include/common.h ../include/mfile.h 
open.o : open.c /usr/include/stdlib.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/stddef.h /usr/include/errno.h /usr/include/linux/errno.h \
  /usr/lib/gcc-lib/i486-linux/2.5.8/include/float.h /usr/include/alloca.h /usr/include/string.h \
  /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h ../include/dict.h \
  ../include/bfile.h ../include/util.h ../include/log.h ../include/options.h ../include/xmalloc.h \
  /usr/include/sys/types.h /usr/include/linux/types.h ../include/res.h ../include/alexpath.h \
  ../include/common.h ../include/mfile.h 
