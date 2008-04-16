
check_PROGRAMS = test_sort1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = test_sort1.cfg test_sort1.idx test_sort1.chr \
 test_sort1_rec.xml sort1.abs

test_sort1_SOURCES = test_sort1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
