
check_PROGRAMS = sort1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg default.idx string-hat.chr \
 rec1.xml rec2.xml rec3.xml rec4.xml zebra.cfg my.abs

sort1_SOURCES = sort1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
