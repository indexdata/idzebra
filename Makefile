# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.13 1994-09-22 10:38:34 quinn Exp $

SHELL=/bin/sh
SUBDIR=util bfile dict

all:
	for i in $(SUBDIR); do cd $$i; if make; then cd ..; else exit 1; fi; done

dep depend:
	for i in $(SUBDIR); do (cd $$i; make dep); done

clean:
	for i in $(SUBDIR); do (cd $$i; make clean); done

cleanup:
	rm -f `find $(SUBDIR) -name "*.[oa]" -print`
	rm -f `find $(SUBDIR) -name "core" -print`
	rm -f `find $(SUBDIR) -name "errlist" -print`
	rm -f `find $(SUBDIR) -name "a.out" -print`

distclean: cleanup clean
	for i in $(SUBDIR); do (cd $$i; \
		mv Makefile Makefile.old; \
		sed '/^#Depend/q' <Makefile.old >Makefile; \
		rm Makefile.old); done
		
wc:
	wc `find . -name '*.[ch]'`
	
