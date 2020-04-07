
check_PROGRAMS = t1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg test.sh 

dist-hook:
	mkdir -p $(distdir)/records
	mkdir -p $(distdir)/tab
	-cp $(srcdir)/records/simple-rusmarc $(distdir)/records
	-cp $(srcdir)/tab/* $(distdir)/tab

t1_SOURCES = t1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZ_CFLAGS)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZ_LIBS)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
