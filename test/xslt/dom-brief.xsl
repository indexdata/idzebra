<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  xmlns:z="http://indexdata.com/zebra-2.0"
  exclude-result-prefixes="m z"
  version="1.0">
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  
  <xsl:template match="text()"/>

  <xsl:template match="m:datafield[@tag='245']">
    <title>
      <xsl:value-of select="m:subfield[@code='a']"/>
    </title>
  </xsl:template>

</xsl:stylesheet>
