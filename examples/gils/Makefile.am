# $Id: Makefile.am,v 1.3 2004-03-09 19:42:02 adam Exp $

EXTRA_DIST = zebra.cfg 

clean:
	rm -f *.mf *..LCK zebrasrv.pid

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records
