<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 version="1.0">
<!-- Identity transform stylesheet -->

<xsl:param name="snippet" select="''"/>
<xsl:output indent="yes"
      method="xml"
      version="1.0"
      encoding="UTF-8"/>

 <xsl:template match="/">
   <snippet>
     <xsl:copy-of select="$snippet"/>
   </snippet>
 </xsl:template>

</xsl:stylesheet>
