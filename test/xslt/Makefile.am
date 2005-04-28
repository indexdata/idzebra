# $Id: Makefile.am,v 1.2 2005-04-28 12:34:45 adam Exp $

check_PROGRAMS = xslt1 xslt2
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg marc-col.xml marc1.xsl

xslt1_SOURCES = xslt1.c
xslt2_SOURCES = xslt2.c

AM_CPPFLAGS = -I$(srcdir)/../api -I$(top_srcdir)/include $(YAZINC)

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

