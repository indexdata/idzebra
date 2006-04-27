# $Id: Makefile.am,v 1.5 2006-04-27 20:40:42 marc Exp $

check_PROGRAMS = t1 t2

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg record.abs m1.xml m2.xml m3.xml sample-marc

t1_SOURCES = t1.c
t2_SOURCES = t2.c

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../api $(YAZINC)

zebralibs = \
 ../../index/libidzebra-api.la \
 ../../rset/libidzebra-rset.la \
 ../../recctrl/libidzebra-recctrl.la \
 ../../dict/libidzebra-dict.la  \
 ../../isams/libidzebra-isams.la \
 ../../isamc/libidzebra-isamc.la \
 ../../isamb/libidzebra-isamb.la \
 ../../data1/libidzebra-data1.la \
 ../../bfile/libidzebra-bfile.la \
 ../../dfa/libidzebra-dfa.la \
 ../../util/libidzebra-util.la

LDADD = ../api/libtestlib.a $(zebralibs) $(YAZLALIB)


clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
