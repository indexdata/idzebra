<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
                xmlns:z="http://indexdata.dk/zebra/xslt/1" 
                xmlns:a="http://alvis.info/enriched/" 
                xmlns:m="http://www.loc.gov/MARC21/slim"
                version="1.0">

  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  <!-- <xsl:include href="xpath.xsl"/> -->
  

  <!-- disable all default text node output -->
  <xsl:template match="text()"/>


  <!-- match on marcxml record -->
  <xsl:template match="//m:record">
    <z:record>
      <!-- <xsl:attribute name="id"></xsl:attribute> -->
      <xsl:attribute name="type">update</xsl:attribute>
      <!-- <xsl:attribute name="rank"></xsl:attribute> -->

      RECORD

    </z:record>
  </xsl:template>


</xsl:stylesheet>
