# $Id: Makefile.am,v 1.1 2004-12-14 09:47:55 adam Exp $

check_PROGRAMS = t1

TESTS = $(check_PROGRAMS)

EXTRA_DIST = zebra.cfg rec1.xml root.abs brief.est

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

