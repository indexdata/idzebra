# $Id: Makefile.am,v 1.5 2004-12-04 00:51:39 adam Exp $

check_PROGRAMS = t1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg test.sh 

dist-hook:
	mkdir -p $(distdir)/records
	mkdir -p $(distdir)/tab
	-cp -p $(srcdir)/records/* $(distdir)/records
	-cp -p $(srcdir)/tab/* $(distdir)/tab

t1_SOURCES = t1.c

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

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

