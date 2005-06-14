# $Id: Makefile.am,v 1.1 2005-06-14 20:03:02 adam Exp $

check_PROGRAMS = text grs.xml grs.marc

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg record.xml record.mrc

text_SOURCES = text.c
grs_xml_SOURCES = grs.xml.c
grs_marc_SOURCES = grs.marc.c

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


