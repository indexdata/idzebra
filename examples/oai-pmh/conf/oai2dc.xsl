<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dc="http://purl.org/dc/elements/1.1/"
  xmlns:oai="http://www.openarchives.org/OAI/2.0/" 
  xmlns:oai_dc="http://www.openarchives.org/OAI/2.0/oai_dc/" 
  exclude-result-prefixes="oai oai_dc"
  version="1.0">

  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  
  <!-- disable all default text node output -->
  <xsl:template match="text()"/>
  
  <!-- match on oai xml record -->
  <xsl:template match="/">
    <dc:metadata>
       <xsl:apply-templates/>
    </dc:metadata>
  </xsl:template>

  <!--
  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/node()">
      <xsl:copy-of select="."/>
  </xsl:template>
  -->

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:date
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:date">
      <dc:date><xsl:value-of select="."/></dc:date>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:description
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:description">
      <dc:description><xsl:value-of select="."/></dc:description>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:format
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:format">
      <format><xsl:value-of select="."/></format>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:identifier
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:identifier">
      <dc:identifier><xsl:value-of select="."/></dc:identifier>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:publisher
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:publisher">
      <dc:publisher><xsl:value-of select="."/></dc:publisher>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:relation
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:relation">
      <dc:relation><xsl:value-of select="."/></dc:relation>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:subject
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:subject">
      <subject><xsl:value-of select="."/></subject>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:title
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:title">
      <dc:title><xsl:value-of select="."/></dc:title>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:type
  | oai:record/oai:metadata/oai_dc:dc/oai_dc:type">
      <dc:type><xsl:value-of select="."/></dc:type>
  </xsl:template>

</xsl:stylesheet>
