# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.8 1994-09-01 15:36:43 quinn Exp $

SHELL=/bin/sh
SUBDIR=util bfile dict

all:
	for i in $(SUBDIR); do cd $$i; if make; then cd ..; else exit 1; fi; done

dep depend:
	for i in $(SUBDIR); do (cd $$i; echo >.depend; make dep); done

clean:
	for i in $(SUBDIR); do (cd $$i; make clean); done

cleanup:
	rm -f `find $(SUBDIR) -name "*.o" -print`
	rm -f `find $(SUBDIR) -name "core" -print`
	rm -f `find $(SUBDIR) -name "errlist" -print`

wc:
	wc `find . -name '*.[ch]'`
	
