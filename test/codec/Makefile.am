# $Id: Makefile.am,v 1.3 2004-08-30 21:36:53 adam Exp $

TESTS = tstcodec

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

tstcodec_SOURCES = tstcodec.c

noinst_PROGRAMS = tstcodec

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = $(zebralibs) $(YAZLALIB) $(TCL_LIB)
