<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  xmlns:z="http://indexdata.com/zebra-2.0"
  exclude-result-prefixes="m z"
  version="1.0">
  <!-- $Id: dom-index-element-chop.xsl,v 1.1 2007-03-08 11:24:50 marc Exp $ -->
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  

  <xsl:template match="text()"/>


  <xsl:template match="/m:record">
    <z:record z:id="{normalize-space(m:controlfield[@tag='001'])}"
        z:rank="{normalize-space(m:rank)}">
      <xsl:apply-templates/>
    </z:record>
  </xsl:template>

  <xsl:template match="m:controlfield[@tag='001']">
    <z:index name="control">
      <xsl:value-of select="normalize-space(.)"/>
    </z:index>
  </xsl:template>
  
  <xsl:template match="m:datafield[@tag='245']">
    <xsl:variable name="chop">
      <xsl:choose>
        <xsl:when test="not(number(@ind2))">0</xsl:when>
        <xsl:otherwise><xsl:value-of select="number(@ind2)"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>  

    <z:index name="title:w title:p any:w">
      <xsl:value-of select="m:subfield[@code='a']"/>
    </z:index>

    <z:index name="title:s">
      <xsl:value-of select="substring(m:subfield[@code='a'], $chop)"/>
    </z:index>

  </xsl:template>

</xsl:stylesheet>
