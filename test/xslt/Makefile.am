# $Id: Makefile.am,v 1.1 2005-04-28 08:20:41 adam Exp $

check_PROGRAMS = xslt1
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg marc-col.xml marc1.xsl

xslt1_SOURCES = xslt1.c

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

