<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  xmlns:z="http://indexdata.com/zebra-2.0"
  exclude-result-prefixes="m z"
  version="1.0">
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  

  <xsl:template match="text()"/>

  <!-- skipping exactly first record with controlfield '11224466' -->
  <xsl:template match="/m:record[normalize-space(m:controlfield[@tag='001']) =
    '11224466']"/>

  <!-- skipping exactly last record with controlfield '73090924' -->
  <xsl:template match="/m:record[normalize-space(m:controlfield[@tag='001']) =
    '73090924']"/>

  <!-- and allowing one specific middle record -->
  <xsl:template match="/m:record[normalize-space(m:controlfield[@tag='001']) =
    '11224467']">
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
  
  <xsl:template match="m:datafield[@tag='245']/m:subfield[@code='a']">
    <z:index name="title:w title:p title:s any:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

</xsl:stylesheet>
