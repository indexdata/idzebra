# $Id: Makefile.am,v 1.26 2004-11-29 21:55:28 adam Exp $

noinst_PROGRAMS = testclient
testclient_SOURCES = testclient.c 

simpletests = t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 
xpathtests= xpath1 xpath2 xpath3 xpath4 xpath5
safaritests = safari1
check_PROGRAMS = $(simpletests) $(xpathtests) $(safaritests)
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebra6.cfg zebra8.cfg zebrazv.cfg \
 zebraxpath.cfg safari.cfg

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

xpath1_SOURCES = xpath1.c
xpath2_SOURCES = xpath2.c
xpath3_SOURCES = xpath3.c
xpath4_SOURCES = xpath4.c
xpath5_SOURCES = xpath5.c

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

