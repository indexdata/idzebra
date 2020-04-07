
check_PROGRAMS = mbox1
TESTS = $(check_PROGRAMS)

mailboxfiles = mail1.mbx mail3.mbx invalid.mbx
EXTRA_DIST= zebra.cfg email2.flt $(mailboxfiles)

mbox1_SOURCES = mbox1.c

AM_CPPFLAGS = -I$(srcdir)/../api -I$(top_srcdir)/include $(YAZ_CFLAGS)

LDADD = ../api/libtestlib.a ../../$(main_zebralib) $(YAZ_LIBS)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
