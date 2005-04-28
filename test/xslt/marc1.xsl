<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  xmlns:z="http://indexdata.dk/zebra/indexing/1"
  version="1.0">
  <!-- $Id: marc1.xsl,v 1.2 2005-04-28 13:34:05 adam Exp $ -->
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  
  <xsl:template match="/">
    <xsl:choose>
      <xsl:when test="$schema='http://indexdata.dk/zebra/indexing/1'">
	<xsl:apply-templates mode="index"/>
      </xsl:when>
      <xsl:when test="$schema='brief'">
	<xsl:apply-templates mode="brief"/>
      </xsl:when>
    </xsl:choose>
  </xsl:template>
  
  <xsl:template match="/m:record/m:controlfield[@tag=001]" mode="index">
    <z:index field="control">
      <xsl:apply-templates match="."/>
    </z:index>
  </xsl:template>
  
  <xsl:template match="/m:record/m:datafield[@tag=245]" mode="index">
    <z:index field="title">
      <xsl:apply-templates match="."/>
    </z:index>
  </xsl:template>
  
  <xsl:template match="/m:record/m:datafield[@tag=245]" mode="brief">
    <title><xsl:value-of select="."/></title>
  </xsl:template>
  
</xsl:stylesheet>
