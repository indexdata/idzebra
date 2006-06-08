<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns:z="http://indexdata.dk/zebra/xslt/1" 
    xmlns:marc="http://www.loc.gov/MARC21/slim" 
    version="1.0">

  <xsl:include href="http://www.loc.gov/marcxml/xslt/MARC21slimUtils.xsl"/>
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>

  <!-- disable all default text node output -->
  <xsl:template match="text()"/>

  <xsl:template match="/">
    <xsl:if test="marc:collection">
      <collection>
        <xsl:apply-templates select="marc:record"/>
      </collection>
    </xsl:if>
    <xsl:if test="marc:record">
       <xsl:apply-templates select="marc:record"/>
    </xsl:if>
  </xsl:template>


  <!-- match on marcxml record -->
  <xsl:template match="marc:record">
    <xsl:variable name="controlField001" select="marc:controlfield[@tag=001]"/>
    <xsl:variable name="controlField008" select="marc:controlfield[@tag=008]"/>
    <z:record id="{$controlField001}" type="update">
      <!-- <xsl:attribute name="id"></xsl:attribute> -->
      <!-- <xsl:attribute name="type">update</xsl:attribute> -->
      <!-- <xsl:attribute name="rank"></xsl:attribute> -->
    </z:record>
  </xsl:template>
</xsl:stylesheet>
