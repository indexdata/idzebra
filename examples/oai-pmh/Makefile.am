# $Id: Makefile.am,v 1.1 2008-02-01 09:55:30 marc Exp $

EXTRA_DIST = README \
   conf/yazserver.xml \
   conf/cql2pqf.txt \
   conf/explain.xml \
   conf/explain2cqlpqftxt.xsl \
   conf/oai2dc.xsl \
   conf/oai2index.xsl \
   conf/oai2zebra.xsl \
   conf/zebra.cfg \
   data/oai-caltech.xml 


conf/cql2pqf.txt: conf/explain2cqlpqftxt.xsl conf/explain.xml
	xsltproc conf/explain2cqlpqftxt.xsl conf/explain.xml > conf/cql2pqf.txt

clean:
	rm -f tmp/*.mf tmp/*..LCK zebrasrv.pid

dist-hook:
	-mkdir $(distdir)/tmp

#	-mkdir $(distdir)/data
#	cp $(srcdir)/data/*.xml $(distdir)/data/*.xml
