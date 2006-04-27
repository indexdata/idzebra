# $Id: Makefile.am,v 1.3 2006-04-27 20:40:42 marc Exp $

check_SCRIPTS = test1.sh test2.sh
TESTS = test1.sh test2.sh

EXTRA_DIST = zebra.cfg $(check_SCRIPTS) g.rec g.abs m.rec m.flt m.rec

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
