# $Id: Makefile.am,v 1.31 2005-12-15 13:28:46 adam Exp $

noinst_PROGRAMS = testclient
testclient_SOURCES = testclient.c 

simpletests = t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 t11 t12 t13 t14
safaritests = safari1
check_PROGRAMS = $(simpletests) $(safaritests)
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebra6.cfg zebra8.cfg zebrazv.cfg safari.cfg

noinst_LIBRARIES = libtestlib.a

libtestlib_a_SOURCES = testlib.c testlib.h

t1_SOURCES = t1.c
t2_SOURCES = t2.c
t3_SOURCES = t3.c
t4_SOURCES = t4.c
t5_SOURCES = t5.c
t6_SOURCES = t6.c
t7_SOURCES = t7.c
t8_SOURCES = t8.c
t9_SOURCES = t9.c rankingrecords.h
t10_SOURCES = t10.c rankingrecords.h
t11_SOURCES = t11.c
t12_SOURCES = t12.c
t13_SOURCES = t13.c
t14_SOURCES = t14.c

safari1_SOURCES = safari1.c testlib.c

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

LDADD = libtestlib.a $(zebralibs) $(YAZLALIB)

