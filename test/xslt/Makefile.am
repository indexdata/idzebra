# $Id: Makefile.am,v 1.13 2007-03-05 13:02:11 marc Exp $

check_PROGRAMS = xslt1 xslt2 xslt3 xslt4 xslt5 dom1
TESTS = $(check_PROGRAMS)

EXTRA_DIST= \
 zebra.cfg zebrastaticrank.cfg zebra-dom.cfg \
 marc-col.xml marc-one.xml marc-col.mrc \
 marc-missing-ns.xml \
 index.xsl id.xsl \
 dom-index-element.xsl  dom-index-pi.xsl \
 marcschema-col.xml marcschema-one.xml snippet.xsl \
 dom-config-col.xml dom-config-one.xml dom-config-marc.xml \
 dom-config-skipped.xml


xslt1_SOURCES = xslt1.c
xslt2_SOURCES = xslt2.c
xslt3_SOURCES = xslt3.c
xslt4_SOURCES = xslt4.c
xslt5_SOURCES = xslt5.c
dom1_SOURCES = dom1.c

AM_CPPFLAGS = -I$(srcdir)/../api -I$(top_srcdir)/include $(YAZINC)

zebralibs = ../../index/libidzebra-2.0.la 

LDADD = ../api/libtestlib.a $(zebralibs) $(YAZLALIB)

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 

