# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.5 1994-08-16 16:33:17 adam Exp $

SUBDIR=util bfile dict

all:
	for i in $(SUBDIR); do (cd $$i; make); done

dep:
	for i in $(SUBDIR); do (cd $$i; echo >.depend; make dep); done

clean:
	for i in $(SUBDIR); do (cd $$i; make clean); done
