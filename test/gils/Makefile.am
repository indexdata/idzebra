
EXTRA_DIST = zebra.cfg test.sh zebra1.cfg zebra2.cfg \
 t1.sh t2.sh t3.sh t4.sh t5.sh t6.sh t7.sh t8.sh

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

TESTS = test01.sh test04.sh test1.sh test2.sh
# FIXME:
# test02.sh fails
# test03.sh fails with a segfault
