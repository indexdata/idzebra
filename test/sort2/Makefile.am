
check_PROGRAMS = test_sort2

TESTS = $(check_PROGRAMS)

EXTRA_DIST = test_sort2.cfg test_sort2.idx \
 test_sort2_rec.xml test_sort2.cfg sort2.abs test_sort2.chr

test_sort2_SOURCES = test_sort2.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

