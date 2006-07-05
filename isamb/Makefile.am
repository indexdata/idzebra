## $Id: Makefile.am,v 1.13 2006-07-05 15:03:48 adam Exp $

noinst_LTLIBRARIES = libidzebra-isamb.la

check_PROGRAMS = tstisamb

TESTS = $(check_PROGRAMS)

tstisamb_SOURCES = tstisamb.c
tstisamb_LDADD = libidzebra-isamb.la \
 ../bfile/libidzebra-bfile.la \
 ../util/libidzebra-util.la $(YAZLALIB)

libidzebra_isamb_la_SOURCES = isamb.c

AM_CPPFLAGS=-I$(srcdir)/../include $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
