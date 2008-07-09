<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:m="http://www.loc.gov/MARC21/slim"
  xmlns:z="http://indexdata.com/zebra-2.0"
  exclude-result-prefixes="m z"
  version="1.0">
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>
  
  <xsl:template match="/*">
    <srw_dc:dc xmlns:srw_dc="info:srw/schema/1/dc-schema"
              xmlns:dc="http://purl.org/dc/elements/1.1/"
	      xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
      <xsl:copy-of select="*"/>
      <z:meta name="snippet"/>
    </srw_dc:dc>
  </xsl:template>

</xsl:stylesheet>
