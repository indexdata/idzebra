# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.7 1994-08-18 11:02:08 adam Exp $

SUBDIR=util bfile dict

all:
	for i in $(SUBDIR); do (cd $$i; make); done

dep depend:
	for i in $(SUBDIR); do (cd $$i; echo >.depend; make dep); done

clean:
	for i in $(SUBDIR); do (cd $$i; make clean); done

wc:
	wc `find . -name '*.[ch]'`
	
