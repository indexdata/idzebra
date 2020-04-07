
check_PROGRAMS = charmap1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg x.xml x.abs default.idx string.utf8.chr

charmap1_SOURCES = charmap1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZ_CFLAGS)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZ_LIBS)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 


