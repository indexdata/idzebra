<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>

  <!-- match on alvis xml record -->
  <xsl:template match="/">
      <xsl:copy-of select="/"/>
  </xsl:template>


</xsl:stylesheet>
