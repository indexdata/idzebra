# $Id: Makefile.am,v 1.25 2004-11-01 11:45:39 heikki Exp $

noinst_PROGRAMS = testclient
testclient_SOURCES = testclient.c 

simpletests = t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 
xpathtests= xpath1 xpath2 xpath3 xpath4 xpath5
#check_PROGRAMS = t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 xpath1
check_PROGRAMS = ${simpletests} ${xpathtests}
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebra6.cfg zebra8.cfg zebrazv.cfg zebraxpath.cfg

t1_SOURCES = t1.c testlib.c testlib.h
t2_SOURCES = t2.c testlib.c testlib.h
t3_SOURCES = t3.c testlib.c testlib.h
t4_SOURCES = t4.c testlib.c testlib.h
t5_SOURCES = t5.c testlib.c testlib.h
t6_SOURCES = t6.c testlib.c testlib.h
t7_SOURCES = t7.c testlib.c testlib.h
t8_SOURCES = t8.c testlib.c testlib.h
t9_SOURCES = t9.c testlib.c testlib.h rankingrecords.h
t10_SOURCES = t10.c testlib.c testlib.h rankingrecords.h

xpath1_SOURCES = xpath1.c testlib.c testlib.h
xpath2_SOURCES = xpath2.c testlib.c testlib.h
xpath3_SOURCES = xpath3.c testlib.c testlib.h
xpath4_SOURCES = xpath4.c testlib.c testlib.h
xpath5_SOURCES = xpath5.c testlib.c testlib.h

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

