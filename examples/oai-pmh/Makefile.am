
oaidatadir = $(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)-examples/oai-pmh
oaiconfdatadir = $(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)-examples/oai-pmh/conf
oaidatadatadir = $(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)-examples/oai-pmh/data

oaidata_DATA = README 

oaiconfdata_DATA = \
   conf/cql2pqf.txt \
   conf/dom-conf.xml \
   conf/explain2cqlpqftxt.xsl \
   conf/explain.xml \
   conf/oai2dc.xsl \
   conf/oai2index.xsl \
   conf/oai2zebra.xsl \
   conf/yazserver.xml \
   conf/zebra.cfg 

oaidatadata_DATA = data/debug-record.xml \
   data/debug-utf8-record.xml \
   data/fetch_OAI_data.sh \
   data/oai-caltech.xml 

EXTRA_DIST = $(oaidata_DATA) $(oaiconfdata_DATA) $(oaidatadata_DATA)

conf/cql2pqf.txt: conf/explain2cqlpqftxt.xsl conf/explain.xml
	xsltproc conf/explain2cqlpqftxt.xsl conf/explain.xml > conf/cql2pqf.txt

clean:
	rm -f tmp/*.mf tmp/*..LCK zebrasrv.pid

