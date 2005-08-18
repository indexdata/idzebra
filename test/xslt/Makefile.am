# $Id: Makefile.am,v 1.7 2005-08-18 12:50:18 adam Exp $

check_PROGRAMS = xslt1 xslt2 xslt3 xslt4
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebrastaticrank.cfg \
 marc-col.xml marc-one.xml index.xsl id.xsl \
 marcschema-col.xml marcschema-one.xml snippet.xsl

xslt1_SOURCES = xslt1.c
xslt2_SOURCES = xslt2.c
xslt3_SOURCES = xslt3.c
xslt4_SOURCES = xslt4.c

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

