## $Id: Makefile.am,v 1.15 2006-12-10 11:54:29 adam Exp $

noinst_LTLIBRARIES = libidzebra-isamb.la

noinst_PROGRAMS = benchisamb
check_PROGRAMS = tstisamb

EXTRA_DIST = bench1.sh bench1.plt bench2.sh bench2.plt

TESTS = $(check_PROGRAMS)

tstisamb_SOURCES = tstisamb.c
tstisamb_LDADD = libidzebra-isamb.la \
 ../bfile/libidzebra-bfile.la \
 ../util/libidzebra-util.la $(YAZLALIB)

benchisamb_SOURCES = benchisamb.c
benchisamb_LDADD = libidzebra-isamb.la \
 ../bfile/libidzebra-bfile.la \
 ../util/libidzebra-util.la $(YAZLALIB)

libidzebra_isamb_la_SOURCES = isamb.c

AM_CPPFLAGS=-I$(srcdir)/../include $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
