
EXTRA_DIST = zebra.cfg test.sh

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

test:
	../../index/zmbolidx init
	../../index/zmbolidx update records
