# $Id: Makefile.am,v 1.2 2004-08-24 10:37:19 adam Exp $

TESTS = tstcodec

zebralibs = \
 ../../rset/libidzebra-rset.la \
 ../../dict/libidzebra-dict.la  \
 ../../isams/libidzebra-isams.la \
 ../../isamc/libidzebra-isamc.la \
 ../../isamb/libidzebra-isamb.la \
 ../../recctrl/libidzebra-recctrl.la \
 ../../data1/libidzebra-data1.la \
 ../../bfile/libidzebra-bfile.la \
 ../../dfa/libidzebra-dfa.la \
 ../../util/libidzebra-util.la \
 ../../index/libidzebra-api.la

tstcodec_SOURCES = tstcodec.c

noinst_PROGRAMS = tstcodec

AM_CPPFLAGS = -I$(top_srcdir)/include $(YAZINC)

LDADD = $(zebralibs) $(YAZLALIB) $(TCL_LIB)
