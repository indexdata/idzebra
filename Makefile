# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.3 1994-08-16 16:21:47 adam Exp $

SUBDIR=util bfile

all:
	for i in $(SUBDIR); do (cd $$i; make); done

dep:
	for i in $(SUBDIR); do (cd $$i; echo >.depend; make dep); done
