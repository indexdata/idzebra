# $Id: Makefile.am,v 1.9 2006-07-03 14:27:35 adam Exp $

check_PROGRAMS = xslt1 xslt2 xslt3 xslt4
TESTS = $(check_PROGRAMS)

EXTRA_DIST=zebra.cfg zebrastaticrank.cfg \
 marc-col.xml marc-one.xml index.xsl id.xsl \
 marcschema-col.xml marcschema-one.xml snippet.xsl

xslt1_SOURCES = xslt1.c
xslt2_SOURCES = xslt2.c
xslt3_SOURCES = xslt3.c
xslt4_SOURCES = xslt4.c

AM_CPPFLAGS = -I$(srcdir)/../api -I$(top_srcdir)/include $(YAZINC)

zebralibs = ../../index/libidzebra-2.0.la 

LDADD = ../api/libtestlib.a $(zebralibs) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

