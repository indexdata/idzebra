# $Id: Makefile.am,v 1.3 2008-02-01 11:05:03 marc Exp $

oaidatadir = $(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)-examples/oai-pmh
oaiconfdatadir = $(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)-examples/oai-pmh/conf
oaidatadatadir = $(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)-examples/oai-pmh/data

oaidata_DATA = README 

oaiconfdata_DATA = conf/yazserver.xml \
   conf/cql2pqf.txt \
   conf/explain.xml \
   conf/explain2cqlpqftxt.xsl \
   conf/oai2dc.xsl \
   conf/oai2index.xsl \
   conf/oai2zebra.xsl \
   conf/zebra.cfg 

oaidatadata_DATA = data/debug-record.xml \
   data/debug-utf8-record.xml \
   data/oai-caltech.xml 

EXTRA_DIST = $(oaidata_DATA) $(oaiconfdata_DATA) $(oaidatadata_DATA)

conf/cql2pqf.txt: conf/explain2cqlpqftxt.xsl conf/explain.xml
	xsltproc conf/explain2cqlpqftxt.xsl conf/explain.xml > conf/cql2pqf.txt

clean:
	rm -f tmp/*.mf tmp/*..LCK zebrasrv.pid

dist-hook:
	-mkdir $(distdir)/tmp

#	-mkdir $(distdir)/data
#	cp $(srcdir)/data/*.xml $(distdir)/data/*.xml
