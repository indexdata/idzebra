

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

TESTS = test01.sh test02.sh test03.sh test04.sh \
        test1.sh test2.sh \
	timing1.sh

EXTRA_DIST = $(TESTS) zebra.cfg zebra1.cfg zebra2.cfg

# FIXME:
# test02.sh fails
# test03.sh fails with a segfault
