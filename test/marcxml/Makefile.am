
check_PROGRAMS = t1 t2

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg record.abs m1.xml m2.xml m3.xml sample-marc

t1_SOURCES = t1.c
t2_SOURCES = t2.c

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../api $(YAZINC)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
