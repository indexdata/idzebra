
EXTRA_DIST = zebra.cfg test.sh zebra1.cfg zebra2.cfg

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

test:
	../../index/zmbolidx init
	../../index/zmbolidx update records

TESTS = t1.sh t2.sh t3.sh t4.sh t5.sh t6.sh t7.sh t8.sh
