<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:pgterms="http://www.gutenberg.org/rdfterms/"
  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:dcterms="http://purl.org/dc/terms/"
  xmlns:dc="http://purl.org/dc/elements/1.1/"
  exclude-result-prefixes="pgterms rdf dcterms">

  <xsl:output method="xml" indent="yes"/>

  <!-- Record element -->
  <xsl:template match="/pgterms:etext">
    <srw_dc:dc
	xmlns:srw_dc="info:srw/schema/1/dc-schema"
	xmlns:dc="http://purl.org/dc/elements/1.1/"
	xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	xsi:schemaLocation="info:srw/schema/1/dc-schema
	http://www.loc.gov/standards/sru/dc-schema.xsd">

      <!-- Generate GP permalink -->
      <dc:identifier>
        <xsl:text>http://www.gutenberg.org/etext/</xsl:text>
	<xsl:value-of select="substring(@rdf:ID, 6)"/>
      </dc:identifier>

      <xsl:apply-templates/>

    </srw_dc:dc>
  </xsl:template>

  <!-- Ignore other elements by mapping into empty DOM XML trees -->
  <xsl:template match="/*"/>

  <!-- Any DC element (except special cases below -->
  <xsl:template match="/pgterms:etext/*[namespace-uri() = 'http://purl.org/dc/elements/1.1/']">
    <xsl:choose>
      <xsl:when test="rdf:Bag">
        <xsl:variable name="myname" select="name()"/>
        <xsl:for-each select="rdf:Bag/*">
	  <xsl:call-template name="cond-display">
	    <xsl:with-param name="name" select="$myname"/>
	    <xsl:with-param name="value" select="."/>
	  </xsl:call-template>
	</xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="cond-display">
	  <xsl:with-param name="name" select="name()"/>
	  <xsl:with-param name="value" select="."/>
	</xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Display this element only if we know and like what kind of content it has -->
  <xsl:template name="cond-display">
    <xsl:param name="name"/>
    <xsl:param name="value"/>

    <xsl:choose>
      <xsl:when test="$value/text()">
        <xsl:element name="{$name}">
	  <xsl:value-of select="normalize-space($value)"/>
	</xsl:element>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
	  <xsl:when test="dcterms:LCSH or dcterms:W3CDTF or dcterms:ISO639-2">
	    <xsl:element name="{$name}">
	      <xsl:value-of select="normalize-space($value)"/>
	    </xsl:element>
	  </xsl:when>
	  <xsl:otherwise>
	    <unknown-type name="{$name}" type="{local-name($value/*)}">
	      <xsl:value-of select="normalize-space($value)"/>
	    </unknown-type>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="//dc:rights">
    <dc:rights>
      <xsl:value-of select="@rdf:resource"/>
    </dc:rights>
  </xsl:template>

  <!-- This is hardly a DC element -->
  <xsl:template match="//dc:tableOfContents">
    <tableOfContents>
      <xsl:value-of select="."/>
    </tableOfContents>
  </xsl:template>

  <xsl:template match="text()"/>

</xsl:stylesheet>

