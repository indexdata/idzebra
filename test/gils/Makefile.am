

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

TESTS = stop01.sh stop02.sh stop03.sh stop04.sh \
        test1.sh test2.sh \
	timing1.sh timing2.sh

EXTRA_DIST = $(TESTS) zebra1.cfg zebra2.cfg

# FIXME:
# test02.sh fails
# test03.sh fails with a segfault
