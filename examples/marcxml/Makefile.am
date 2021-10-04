marcxmldatadir=$(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)-examples/marcxml

marcxmldata_DATA = zebra.cfg\
   README\
   MARC21slim2INDEX.xsl\
   MARC21slim2MADS.xsl\
   MARC21slim2MODS3-7.xsl\
   MARC21slim2SRWDC.xsl\
   MARC21slimUtils.xsl\
   collection-2.xml\
   collection-clasmrc.xml\
   collection-namemrc.xml\
   collection-opera-43.xml\
   collection-sandburg-1.xml\
   collection-subjmrc.xml\
   dom-config.xml\
   identity.xsl\
   zebra.xsl \
   yazgfs.xml

EXTRA_DIST = $(marcxmldata_DATA)

clean:
	rm -f *.mf *.LCK zebrasrv.pid

