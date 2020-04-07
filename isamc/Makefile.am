
noinst_LTLIBRARIES = libidzebra-isamc.la

libidzebra_isamc_la_SOURCES = isamc.c merge.c isamc-p.h

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../index $(YAZ_CFLAGS)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZ_LIBS)
