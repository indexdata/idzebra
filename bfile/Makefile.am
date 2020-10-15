
noinst_LTLIBRARIES = libidzebra-bfile.la

check_PROGRAMS = tstmfile1 tstbfile2

TESTS = $(check_PROGRAMS)

tstmfile1_SOURCES = tstmfile1.c
tstbfile2_SOURCES = tstbfile2.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

libidzebra_bfile_la_SOURCES = bfile.c mfile.c cfile.c commit.c cfile.h mfile.h

LDADD = libidzebra-bfile.la ../util/libidzebra-util.la $(YAZLALIB)

clean-local:
	-rm -fr *.log *.mf shadow

