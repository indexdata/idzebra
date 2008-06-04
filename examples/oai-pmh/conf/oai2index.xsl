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
    <z:index name="any:w dc_title:w dc_title:p">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:creator
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:creator">
    <z:index name="any:w dc_creator:w dc_creator:p">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:subject
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:subject">
    <z:index name="any:w dc_subject:w dc_subject:p">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:description
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:description">
    <z:index name="any:w dc_description:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:contributor
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:contributor">
    <z:index name="any:w dc_contributor:w dc_contributor:p">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:publisher
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:publisher">
    <z:index name="dc_publisher:p dc_publisher:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:date
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:date">
    <z:index name="dc_date:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:format
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:format">
    <z:index name="dc_format:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:identifier
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:identifier">
    <z:index name="dc_identifier:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:source
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:source">
    <z:index name="dc_source:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:language
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:language">
    <z:index name="dc_language:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:relation
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:relation">
    <z:index name="dc_relation:0">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template match="oai:record/oai:metadata/oai_dc:dc/dc:rights
                    | oai:record/oai:metadata/oai_dc:dc/oai_dc:rights">
    <z:index name="dc_rights:p dc_rights:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

</xsl:stylesheet>
