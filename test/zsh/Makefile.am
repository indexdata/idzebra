
#dist-hook:
#	-mkdir $(distdir)/records
#	cp $(srcdir)/records/*.grs $(distdir)/records


check_SCRIPTS = testall.sh

TESTS = testall.sh

EXTRA_DIST = $(check_SCRIPTS) test1.zsh test2.zsh zebra.cfg 

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
	-rm -rf reg/*.LCK 
	-rm -rf reg/*.log 
	-rm -rf reg/*.mf 

