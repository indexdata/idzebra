
EXTRA_DIST = zebra.cfg 

clean:
	rm -f *.mf *..LCK zebrasrv.pid

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records
