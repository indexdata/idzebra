# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.11 1994-09-12 08:01:41 quinn Exp $

SHELL=/bin/sh
SUBDIR=util bfile dict isam

all:
	for i in $(SUBDIR); do cd $$i; if make; then cd ..; else exit 1; fi; done

dep depend:
	for i in $(SUBDIR); do (cd $$i; make dep); done

clean:
	for i in $(SUBDIR); do (cd $$i; make clean); done

cleanup:
	rm -f `find $(SUBDIR) -name "*.o" -print`
	rm -f `find $(SUBDIR) -name "core" -print`
	rm -f `find $(SUBDIR) -name "errlist" -print`
	rm -f `find $(SUBDIR) -name "a.out" -print`

wc:
	wc `find . -name '*.[ch]'`
	
