

dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.xml $(distdir)/records
	-mkdir $(distdir)/tmp
	-mkdir $(distdir)/lock
	-mkdir $(distdir)/register

TESTS = test.sh

EXTRA_DIST = $(TESTS) zebra.cfg user.abs

