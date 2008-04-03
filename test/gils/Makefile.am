
dist-hook:
	-mkdir $(distdir)/records
	cp $(srcdir)/records/*.grs $(distdir)/records

check_SCRIPTS = stop01.sh stop02.sh stop03.sh stop04.sh \
        test1.sh test2.sh test3.sh timing1.sh timing2.sh

TESTS = $(check_SCRIPTS)

EXTRA_DIST = cleanrecords.sh zebra1.cfg zebra2.cfg $(check_SCRIPTS)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
	-rm -rf reg/*.mf 

