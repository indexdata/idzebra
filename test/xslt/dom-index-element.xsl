<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  xmlns:z="http://indexdata.com/zebra-2.0"
  exclude-result-prefixes="m z"
  version="1.0">
  <!-- $Id: dom-index-element.xsl,v 1.1 2007-02-15 13:01:00 marc Exp $ -->
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  

  <xsl:template match="text()"/>


  <xsl:template match="/m:record">
    <z:record z:id="{normalize-space(m:controlfield[@tag='001'])}"
        z:rank="{normalize-space(m:rank)}"
	>
      <xsl:apply-templates/>
    </z:record>
  </xsl:template>

  <xsl:template match="m:controlfield[@tag='001']">
    <z:index name="control">
      <xsl:value-of select="normalize-space(.)"/>
    </z:index>
  </xsl:template>
  
  <xsl:template match="m:datafield[@tag='245']/m:subfield[@code='a']">
    <!-- nested. does not have to be! -->
    <z:index name="title">
      <z:index name="title" type="p">
        <xsl:value-of select="."/>
      </z:index>
    </z:index>

    <!-- can do. But sort register only supports numeric attributes. -->
    <z:index name="title" type="s"> 
      <xsl:value-of select="."/>
    </z:index>

  </xsl:template>

</xsl:stylesheet>
