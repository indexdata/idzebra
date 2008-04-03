
check_PROGRAMS = tstcodec

TESTS = $(check_PROGRAMS)

tstcodec_SOURCES = tstcodec.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = ../../$(main_zebralib) $(YAZLALIB)
