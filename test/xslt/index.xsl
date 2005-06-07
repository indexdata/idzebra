<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  xmlns:z="http://indexdata.dk/zebra/xslt/1"
  version="1.0">
  <!-- $Id: index.xsl,v 1.2 2005-06-07 13:10:52 adam Exp $ -->
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  
  <xsl:template match="/m:record/m:controlfield[@tag=001]">
    <z:index field="control">
      <xsl:apply-templates match="."/>
    </z:index>
  </xsl:template>
  
  <xsl:template match="/m:record/m:datafield[@tag=245]">
    <z:index field="title">
      <xsl:apply-templates match="."/>
    </z:index>
  </xsl:template>
  
</xsl:stylesheet>
