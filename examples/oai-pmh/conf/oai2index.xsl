<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:z="http://indexdata.com/zebra-2.0"
                xmlns:dc="http://purl.org/dc/elements/1.1/"
                xmlns:oai="http://www.openarchives.org/OAI/2.0/" 
                xmlns:oai_dc="http://www.openarchives.org/OAI/2.0/oai_dc/" 
                exclude-result-prefixes="oai oai_dc dc"
                version="1.0">

  <!-- xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" -->


  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>

  <!-- disable all default text node output -->
  <xsl:template match="text()"/>
  
  <!-- match on oai xml record -->
  <xsl:template match="/">    
    <z:record z:id="{normalize-space(oai:record/oai:header/oai:identifier)}">

      <xsl:apply-templates/>
    </z:record>
  </xsl:template>

  <!-- OAI indexing templates -->
  <xsl:template match="oai:record/oai:header/oai:identifier">
    <z:index name="oai_identifier:0">
      <xsl:value-of select="."/>
    </z:index>    
  </xsl:template>

  <xsl:template match="oai:record/oai:header/oai:datestamp">
    <z:index name="oai_datestamp:0">
      <xsl:value-of select="."/>
    </z:index>    
  </xsl:template>

  <xsl:template match="oai:record/oai:header/oai:setSpec">
    <z:index name="oai_setspec:0">
      <xsl:value-of select="."/>
    </z:index>    
  </xsl:template>

  <!-- DC specific indexing templates -->
  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:title 
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:title">
    <z:index name="any:w title:w title:p">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:creator
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:creator">
    <z:index name="any:w author:w author:p">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:subject
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:subject">
    <z:index name="any:w subject-heading:w subject-heading:p">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:description
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:description">
    <z:index name="any:w description:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:contributor
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:contributor">
    <z:index name="any:w contributor:w contributor:p">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:publisher
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:publisher">
    <z:index name="publisher:p publisher:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:date
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:date">
    <z:index name="date:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:format
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:format">
    <z:index name="format:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:identifier
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:identifier">
    <z:index name="identifier:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:source
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:source">
    <z:index name="source:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:language
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:language">
    <z:index name="language:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:relation
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:relation">
    <z:index name="relation:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:rights
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:rights">
    <z:index name="rights:p rights:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

</xsl:stylesheet>
