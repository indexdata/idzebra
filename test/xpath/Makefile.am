# $Id: Makefile.am,v 1.12 2006-04-27 20:40:43 marc Exp $

check_PROGRAMS = xpath1 xpath2 xpath3 xpath4 xpath5 xpath6

TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg rec1.xml rec2.xml assembled.abs

xpath1_SOURCES = xpath1.c
xpath2_SOURCES = xpath2.c
xpath3_SOURCES = xpath3.c
xpath4_SOURCES = xpath4.c
xpath5_SOURCES = xpath5.c
xpath6_SOURCES = xpath6.c

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

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
