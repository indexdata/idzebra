# $Id: Makefile.am,v 1.21 2004-10-28 15:24:36 heikki Exp $

noinst_PROGRAMS = testclient
testclient_SOURCES = testclient.c

check_PROGRAMS = t1 t2 t3 t4 t5 t6 t7 t8 t9 t10
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebra6.cfg zebra8.cfg

t1_SOURCES = t1.c testlib.c
t2_SOURCES = t2.c testlib.c
t3_SOURCES = t3.c testlib.c
t4_SOURCES = t4.c testlib.c
t5_SOURCES = t5.c testlib.c
t6_SOURCES = t6.c testlib.c
t7_SOURCES = t7.c testlib.c
t8_SOURCES = t8.c testlib.c
t9_SOURCES = t9.c testlib.c
t10_SOURCES = t10.c testlib.c

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

