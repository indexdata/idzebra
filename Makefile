# Copyright (C) 1994-1996, Index Data I/S 
# All rights reserved.
# Sebastian Hammer, Adam Dickmeiss
# $Id: Makefile,v 1.54 1996-05-22 08:26:36 adam Exp $

SHELL=/bin/sh
MAKE=make
RANLIB=ranlib

# Where are Yaz libraries located?
YAZLIB=../../yaz/lib/libyaz.a
#YAZLIB=/usr/local/lib/libyazchkr.a
#YAZLIB=-lyaz
# Where are Yaz header files located?
YAZINC=-I../../yaz/include
# If Yaz is compiled with mosi support uncomment and specify.
#OSILIB=../../xtimosi/src/libmosi.a ../../yaz/lib/librfc.a

# Some systems have seperate socket libraries
#NETLIB=-lnsl -lsocket

SUBDIR=util bfile dfa dict isam rset index

all:
	for i in $(SUBDIR); do cd $$i; if $(MAKE) OSILIB="$(OSILIB)" YAZLIB="$(YAZLIB)" YAZINC="$(YAZINC)" RANLIB="$(RANLIB)" NETLIB="$(NETLIB)" CFLAGS="$(CFLAGS)" CC="$(CC)"; then cd ..; else exit 1; fi; done

dep depend:
	for i in $(SUBDIR); do cd $$i; if $(MAKE) YAZINC="$(YAZINC)" depend; then cd ..; else exit 1; fi; done

clean:
	for i in $(SUBDIR); do (cd $$i; $(MAKE) clean); done
	rm -f lib/*.a

cleanup:
	rm -f `find $(SUBDIR) -name "*.[oa]" -print`
	rm -f `find $(SUBDIR) -name "core" -print`
	rm -f `find $(SUBDIR) -name "errlist" -print`
	rm -f `find $(SUBDIR) -name "a.out" -print`

cleandepend: 
	for i in $(SUBDIR); do (cd $$i; \
		if sed '/^#Depend/q' <Makefile >Makefile.tmp; then \
		mv -f Makefile.tmp Makefile; fi; rm -f .depend); done

taildepend:
	for i in $(SUBDIR); do (cd $$i; \
		if sed 's/^if/#if/' <Makefile|sed 's/^include/#include/'| \
		sed 's/^endif/#endif/' | \
		sed 's/^depend: depend2/depend: depend1/g' | \
		sed '/^#Depend/q' >Makefile.tmp; then \
		mv -f Makefile.tmp Makefile; fi); done

gnudepend:
	for i in $(SUBDIR); do (cd $$i; \
		if sed '/^#Depend/q' <Makefile| \
		sed 's/^#if/if/' |sed 's/^#include/include/'| \
		sed 's/^#endif/endif/' | \
		sed 's/^depend: depend1/depend: depend2/g' >Makefile.tmp;then \
		mv -f Makefile.tmp Makefile; fi); done

wc:
	wc `find . -name '*.[ch]'`
	
