# $Id: Makefile.am,v 1.15 2007-03-06 09:24:34 marc Exp $

check_PROGRAMS = xslt1 xslt2 xslt3 xslt4 xslt5 dom1
TESTS = $(check_PROGRAMS)

EXTRA_DIST= \
     dom-config-col.xml \
     dom-config-marc.xml \
     dom-config-one.xml \
     dom-config-skipped.xml \
     dom-index-element.xsl \
     dom-index-pi.xsl \
     dom-index-skipped.xsl \
     id.xsl \
     index.xsl \
     marc-col.mrc \
     marc-col.xml \
     marc-missing-ns.xml \
     marc-one.xml \
     marcschema-col.xml \
     marcschema-one.xml \
     snippet.xsl \
     zebra-dom.cfg \
     zebra.cfg \
     zebrastaticrank.cfg



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

