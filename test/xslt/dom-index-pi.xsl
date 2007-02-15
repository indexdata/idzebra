<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  exclude-result-prefixes="m"
  version="1.0">
  <!-- $Id: dom-index-pi.xsl,v 1.1 2007-02-15 13:01:00 marc Exp $ -->
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  

  <xsl:template match="text()"/>

  <!--
  <xsl:template match="processing-instruction()">
    <xsl:copy-of select="."/>
  </xsl:template>
  -->

  <xsl:template match="/m:record">
    <xsl:processing-instruction name="zebra-2.0">
      <xsl:text>record id=</xsl:text>
      <xsl:value-of select="normalize-space(m:controlfield[@tag='001'])"/>
      <xsl:text> rank=</xsl:text>
      <xsl:value-of select="normalize-space(m:rank)"/>
    </xsl:processing-instruction>

    <record>
      <xsl:apply-templates/>
    </record>
  </xsl:template>

  <xsl:template match="m:controlfield[@tag='001']">

    <xsl:processing-instruction 
        name="zebra-2.0">index control:w</xsl:processing-instruction>
    <control>
      <xsl:value-of select="normalize-space(.)"/>
    </control>
  </xsl:template>
  
  <xsl:template match="m:datafield[@tag='245']/m:subfield[@code='a']">
    <xsl:processing-instruction 
        name="zebra-2.0">index title:w title:p title:s</xsl:processing-instruction>
    <title>
        <xsl:value-of select="."/>
    </title>
  </xsl:template>

</xsl:stylesheet>
