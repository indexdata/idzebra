<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 version="1.0">
<!-- Identity transform stylesheet -->

<xsl:param name="snippet" select="''"/>
<xsl:param name="score" select="''"/>
<xsl:param name="id" select="''"/>
<xsl:param name="rank" select="''"/>
<xsl:output indent="yes"
      method="xml"
      version="1.0"
      encoding="UTF-8"/>

 <xsl:template match="/">
   <snippet>
     <id><xsl:value-of select="$id"/></id>
     <score><xsl:value-of select="$score"/></score>
     <rank><xsl:value-of select="$rank"/></rank>
     <xsl:copy-of select="$snippet"/>
   </snippet>
 </xsl:template>

</xsl:stylesheet>
