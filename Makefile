# Copyright (C) 1994, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.2 1994-08-16 16:15:48 adam Exp $

SUBDIR=util 

all:
	for i in $(SUBDIR); do (cd $$i; make); done

dep:
	for i in $(SUBDIR); do (cd $$i; make dep); done
