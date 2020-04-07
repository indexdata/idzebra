
noinst_LTLIBRARIES = libidzebra-isamb.la

noinst_PROGRAMS = benchisamb benchindex1
check_PROGRAMS = tstisamb

EXTRA_DIST = bench1.sh bench1.plt bench2.sh bench2.plt \
 filesystems.plt dictisam.plt

TESTS = $(check_PROGRAMS)

tstisamb_SOURCES = tstisamb.c
tstisamb_LDADD = libidzebra-isamb.la \
 ../bfile/libidzebra-bfile.la \
 ../util/libidzebra-util.la $(YAZ_LIBS)

benchisamb_SOURCES = benchisamb.c
benchisamb_LDADD = libidzebra-isamb.la \
 ../bfile/libidzebra-bfile.la \
 ../util/libidzebra-util.la $(YAZ_LIBS)

benchindex1_SOURCES = benchindex1.c
benchindex1_LDADD = libidzebra-isamb.la \
 ../bfile/libidzebra-bfile.la \
 ../dict/libidzebra-dict.la \
 ../util/libidzebra-util.la $(YAZ_LIBS)

libidzebra_isamb_la_SOURCES = isamb.c

AM_CPPFLAGS=-I$(srcdir)/../include $(YAZ_CFLAGS)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZ_LIBS)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
