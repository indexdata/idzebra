<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  xmlns:z="http://indexdata.dk/zebra/xslt/1"
  version="1.0">
  <!-- $Id: index.xsl,v 1.4 2005-06-24 13:45:54 adam Exp $ -->
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  

  <xsl:template match="text()"/>


  <xsl:template match="/m:record">
    <z:record id="{normalize-space(m:controlfield[@tag='001'])}">
      <xsl:apply-templates/>
    </z:record>
  </xsl:template>

  <xsl:template match="m:controlfield[@tag='001']">
    <z:index name="control">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>
  
  <xsl:template match="m:datafield[@tag='245']/m:subfield[@code='a']">
    <z:index name="title">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>
  


</xsl:stylesheet>
