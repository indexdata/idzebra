# $Id: Makefile.am,v 1.17 2004-09-30 08:16:49 adam Exp $

noinst_PROGRAMS = testclient
testclient_SOURCES = testclient.c

check_PROGRAMS = t1 t2 t3 t4 t5 t6 t7
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebra6.cfg

t1_SOURCES = t1.c
t2_SOURCES = t2.c
t3_SOURCES = t3.c
t4_SOURCES = t4.c
t5_SOURCES = t5.c
t6_SOURCES = t6.c
t7_SOURCES = t7.c

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

LDADD = $(zebralibs) $(YAZLALIB)

