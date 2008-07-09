<?xml version="1.0"?>

<!-- This maps any root element containing elements in the DC namespace to an index structure
-->

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:pgterms="http://www.gutenberg.org/rdfterms/"
  xmlns:z="http://indexdata.com/zebra-2.0"
  exclude-result-prefixes="pgterms">

  <!-- Extract sort keys in addition to word keys -->
  <xsl:variable name="sort">|title|date|creator|</xsl:variable>
  <!-- Extract phrase keys in addition to word keys -->
  <xsl:variable name="phrase">|title|date|creator|</xsl:variable>

  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/ignore">
    <z:record/>
  </xsl:template>

  <xsl:template match="/*">
    <z:record>
      <z:index name="any:w">
        <xsl:apply-templates/>
      </z:index>

      <xsl:call-template name="special-indexes"/>

      <!--
      <z:index name="anywhere:w">
        <xsl:value-of select="normalize-space()"/>
      </z:index>
      -->
    </z:record>
  </xsl:template>

  <xsl:template match="/*/*[namespace-uri() = 'http://purl.org/dc/elements/1.1/']">
    <z:index name="{local-name()}:w">
      <xsl:value-of select="."/>
    </z:index>
  </xsl:template>

  <xsl:template name="special-indexes">
    <xsl:for-each select="/*/*">
      <xsl:if test="contains($sort, local-name(.))">
	<z:index name="{local-name(.)}:s">
	  <xsl:value-of select="."/>
	</z:index>
      </xsl:if>
      <xsl:if test="contains($phrase, local-name(.))">
	<z:index name="{local-name(.)}:p">
	  <xsl:value-of select="."/>
	</z:index>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="text()"/>
</xsl:stylesheet>
