<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns="http://www.loc.gov/mods/v3" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:marc="http://www.loc.gov/MARC21/slim" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" exclude-result-prefixes="marc">
  
  <xsl:include href="http://www.loc.gov/marcxml/xslt/MARC21slimUtils.xsl"/>
  <xsl:output method="xml" indent="yes" encoding="UTF-8"/>

  <!--
      adapted from MARC21Slim2MODS3.xsl ntra 7/6/05
      revised rsg/jer 11/29/05
  -->

  <!-- authority attribute defaults to 'naf' if not set using this authority parameter, for <authority> descriptors: name, titleInfo, geographic -->
  <xsl:param name="authority"/>
  <xsl:variable name="auth">
    <xsl:choose>
      <xsl:when test="$authority">
	<xsl:value-of select="$authority"/>
      </xsl:when>
      <xsl:otherwise>naf</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="controlField008-11" select="substring(descendant-or-self::marc:controlfield[@tag=008],12,1)"/>
  <xsl:variable name="controlField008-14" select="substring(descendant-or-self::marc:controlfield[@tag=008],15,1)"/>
  <xsl:template match="/">
    <xsl:choose>
      <xsl:when test="descendant-or-self::marc:collection">
	<madsCollection xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.loc.gov/mads http://www.loc.gov/standards/mads/mads.xsd">
	  <xsl:for-each select="descendant-or-self::marc:collection/marc:record">
	    <mads version="beta">
	      <xsl:call-template name="marcRecord"/>
	    </mads>
	  </xsl:for-each>
	</madsCollection>
      </xsl:when>
      <xsl:otherwise>
	<mads version="beta" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.loc.gov/mads http://www.loc.gov/standards/mads/mads.xsd">
	  <xsl:for-each select="descendant-or-self::marc:record">
	    <xsl:call-template name="marcRecord"/>
	  </xsl:for-each>
	</mads>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="marcRecord">		
    <xsl:if test="contains(marc:datafield[@tag='035'],'tea2003000479')">

    </xsl:if>		
    <authority>
      <xsl:apply-templates select="marc:datafield[100 &lt;= @tag  and @tag &lt; 200]"/>
    </authority>
    <!-- related -->
    <xsl:apply-templates select="marc:datafield[500 &lt;= @tag and @tag &lt;= 585]|marc:datafield[700 &lt;= @tag and @tag &lt;= 785]"/>

    <!-- variant -->
    <xsl:apply-templates select="marc:datafield[400 &lt;= @tag and @tag &lt;= 485]"/>
    <!-- notes -->
    <xsl:apply-templates select="marc:datafield[667 &lt;= @tag and @tag &lt;= 688]"/>
    <!-- url -->
    <xsl:apply-templates select="marc:datafield[@tag=856]"/>
    <recordInfo>
      <xsl:apply-templates select="marc:datafield[@tag=010]"/>
      <xsl:apply-templates select="marc:datafield[@tag=024]"/>
      <xsl:apply-templates select="marc:datafield[@tag=040]/marc:subfield[@code='a']"/>
      <xsl:apply-templates select="marc:controlfield[@tag=008]"/>
      <xsl:apply-templates select="marc:controlfield[@tag=005]"/>
      <xsl:apply-templates select="marc:controlfield[@tag=001]"/>
      <xsl:apply-templates select="marc:datafield[@tag=040]/marc:subfield[@code='b']"/>
    </recordInfo>
  </xsl:template>

  <!-- start of secondary templates -->

  <!-- ======== xlink ======== -->

   <!-- <xsl:template name="uri"> 
    <xsl:for-each select="marc:subfield[@code='0']">
      <xsl:attribute name="xlink:href">
	<xsl:value-of select="."/>
      </xsl:attribute>
    </xsl:for-each>
     </xsl:template> 
   -->
  <xsl:template match="marc:subfield[@code='i']">
    <xsl:attribute name="type">
      <xsl:value-of select="."/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template name="role">
    <xsl:for-each select="marc:subfield[@code='e']">
      <role>
	<roleTerm type="text">
	  <xsl:value-of select="."/>
	</roleTerm>
      </role>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="part">
    <xsl:variable name="partNumber">
      <xsl:call-template name="specialSubfieldSelect">
	<xsl:with-param name="axis">n</xsl:with-param>
	<xsl:with-param name="anyCodes">n</xsl:with-param>
	<xsl:with-param name="afterCodes">fghkdlmor</xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="partName">
      <xsl:call-template name="specialSubfieldSelect">
	<xsl:with-param name="axis">p</xsl:with-param>
	<xsl:with-param name="anyCodes">p</xsl:with-param>
	<xsl:with-param name="afterCodes">fghkdlmor</xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="string-length(normalize-space($partNumber))">
      <partNumber>
	<xsl:call-template name="chopPunctuation">
	  <xsl:with-param name="chopString" select="$partNumber"/>
	</xsl:call-template>
      </partNumber>
    </xsl:if>
    <xsl:if test="string-length(normalize-space($partName))">
      <partName>
	<xsl:call-template name="chopPunctuation">
	  <xsl:with-param name="chopString" select="$partName"/>
	</xsl:call-template>
      </partName>
    </xsl:if>
  </xsl:template>

  <xsl:template name="nameABCDN">
    <xsl:for-each select="marc:subfield[@code='a']">
      <namePart>
	<xsl:call-template name="chopPunctuation">
	  <xsl:with-param name="chopString" select="."/>
	</xsl:call-template>
      </namePart>
    </xsl:for-each>
    <xsl:for-each select="marc:subfield[@code='b']">
      <namePart>
	<xsl:value-of select="."/>
      </namePart>
    </xsl:for-each>
    <xsl:if test="marc:subfield[@code='c'] or marc:subfield[@code='d'] or marc:subfield[@code='n']">
      <namePart>
	<xsl:call-template name="subfieldSelect">
	  <xsl:with-param name="codes">cdn</xsl:with-param>
	</xsl:call-template>
      </namePart>
    </xsl:if>
  </xsl:template>

  <xsl:template name="nameABCDQ">
    <namePart>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:call-template name="subfieldSelect">
	    <xsl:with-param name="codes">aq</xsl:with-param>
	  </xsl:call-template>
	</xsl:with-param>
      </xsl:call-template>
    </namePart>
    <xsl:call-template name="termsOfAddress"/>
    <xsl:call-template name="nameDate"/>
  </xsl:template>

  <xsl:template name="nameACDENQ">
    <namePart>
      <xsl:call-template name="subfieldSelect">
	<xsl:with-param name="codes">acdenq</xsl:with-param>
      </xsl:call-template>
    </namePart>
  </xsl:template>

  <xsl:template name="nameDate">
    <xsl:for-each select="marc:subfield[@code='d']">
      <namePart type="date">
	<xsl:call-template name="chopPunctuation">
	  <xsl:with-param name="chopString" select="."/>
	</xsl:call-template>
      </namePart>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="specialSubfieldSelect">
    <xsl:param name="anyCodes"/>
    <xsl:param name="axis"/>
    <xsl:param name="beforeCodes"/>
    <xsl:param name="afterCodes"/>
    <xsl:variable name="str">
      <xsl:for-each select="marc:subfield">
	<xsl:if test="contains($anyCodes, @code)      or (contains($beforeCodes,@code) and following-sibling::marc:subfield[@code=$axis])      or (contains($afterCodes,@code) and preceding-sibling::marc:subfield[@code=$axis])">
	  <xsl:value-of select="text()"/>
	  <xsl:text> </xsl:text>
	</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:value-of select="substring($str,1,string-length($str)-1)"/>
  </xsl:template>

  <xsl:template name="termsOfAddress">
    <xsl:if test="marc:subfield[@code='b' or @code='c']">
      <namePart type="termsOfAddress">
	<xsl:call-template name="chopPunctuation">
	  <xsl:with-param name="chopString">
	    <xsl:call-template name="subfieldSelect">
	      <xsl:with-param name="codes">bc</xsl:with-param>
	    </xsl:call-template>
	  </xsl:with-param>
	</xsl:call-template>
      </namePart>
    </xsl:if>
  </xsl:template>

  <xsl:template name="displayLabel">
    <xsl:if test="marc:subfield[@code='z']">
      <xsl:attribute name="displayLabel">
	<xsl:value-of select="marc:subfield[@code='z']"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="marc:subfield[@code='3']">
      <xsl:attribute name="displayLabel">
	<xsl:value-of select="marc:subfield[@code='3']"/>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <xsl:template name="isInvalid">
    <xsl:if test="marc:subfield[@code='z']">
      <xsl:attribute name="invalid">yes</xsl:attribute>
    </xsl:if>
  </xsl:template>

  <xsl:template name="sub2Attribute">
    <!-- 024 -->
    <xsl:if test="marc:subfield[@code='2']">
      <xsl:attribute name="type">
	<xsl:value-of select="marc:subfield[@code='2']"/>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>
  <xsl:template match="marc:controlfield[@tag=001]">
    <recordIdentifier>
      <xsl:if test="../marc:controlfield[@tag=003]">
	<xsl:attribute name="source">
	  <xsl:value-of select="../marc:controlfield[@tag=003]"/>
	</xsl:attribute>
      </xsl:if>
      <xsl:value-of select="."/>
    </recordIdentifier>
  </xsl:template>

  <xsl:template match="marc:controlfield[@tag=005]">
    <recordChangeDate encoding="iso8601">
      <xsl:value-of select="."/>
    </recordChangeDate>
  </xsl:template>

  <xsl:template match="marc:controlfield[@tag=008]">
    <recordCreationDate encoding="marc">
      <xsl:value-of select="substring(.,1,6)"/>
    </recordCreationDate>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=010]">
    <identifier>
      <xsl:call-template name="isInvalid"/>
      <xsl:call-template name="sub2Attribute"/>
      <xsl:value-of select="marc:subfield[@code='a']"/>
    </identifier>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=024]">
    <identifier>
      <xsl:call-template name="isInvalid"/>
      <xsl:call-template name="sub2Attribute"/>
      <xsl:value-of select="marc:subfield[@code='a']"/>
    </identifier>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=040]/marc:subfield[@code='a']">
    <recordContentSource authority="marcorg">
      <xsl:value-of select="."/>
    </recordContentSource>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=040]/marc:subfield[@code='b']">
    <languageOfCataloging>
      <languageTerm authority="iso639-2b" type="code">
	<xsl:value-of select="."/>
      </languageTerm>
    </languageOfCataloging>
  </xsl:template>
  
  <!-- ========== names  ========== -->
  <xsl:template match="marc:datafield[@tag=100]">
    <name type="personal"> 			
      <xsl:call-template name="setAuthority"/>
      <xsl:call-template name="nameABCDQ"/>
      <xsl:call-template name="role"/>		
    </name>
    <xsl:apply-templates select="*[marc:subfield[not(contains('abcdeq',@code))]]"/>
    <xsl:call-template name="title"/>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=110]">
    <name type="corporate">
      <xsl:call-template name="setAuthority"/>
      <xsl:call-template name="nameABCDN"/>
      <xsl:call-template name="role"/>
    </name>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=111]">
    <name type="conference">
      <xsl:call-template name="setAuthority"/>			
      <xsl:call-template name="nameACDENQ"/>
    </name>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=400]">
    <variant>
      <xsl:call-template name="variantTypeAttribute"/>
      <name type="personal">
	<xsl:call-template name="nameABCDQ"/>
	<xsl:call-template name="role"/>
      </name>
      <xsl:apply-templates/>
      <xsl:call-template name="title"/>
    </variant>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=410]">
    <variant>
      <xsl:call-template name="variantTypeAttribute"/>
      <name type="corporate">
	<xsl:call-template name="nameABCDN"/>
	<xsl:call-template name="role"/>				
      </name>
      <xsl:apply-templates/>
    </variant>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=411]">
    <variant>
      <xsl:call-template name="variantTypeAttribute"/>
      <name type="conference">
	<xsl:call-template name="nameACDENQ"/>
      </name>
      <xsl:apply-templates/>
    </variant>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=500]|marc:datafield[@tag=700]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <!-- <xsl:call-template name="uri"/> -->		
      <name type="personal">
	<xsl:call-template name="setAuthority"/>								
	<xsl:call-template name="nameABCDQ"/>
	<xsl:call-template name="role"/>				
      </name>
      <xsl:call-template name="title"/>
      <xsl:apply-templates/>
    </related>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=510]|marc:datafield[@tag=710]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <!-- <xsl:call-template name="uri"/> -->
      <name type="corporate">
	<xsl:call-template name="setAuthority"/>
	<xsl:call-template name="nameABCDN"/>
	<xsl:call-template name="role"/>
      </name>
      <xsl:apply-templates/>
    </related>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=511]|marc:datafield[@tag=711]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <!-- <xsl:call-template name="uri"/> -->
      <name type="conference">
	<xsl:call-template name="setAuthority"/>
	<xsl:call-template name="nameACDENQ"/>
      </name>
      <xsl:apply-templates/>
    </related>
  </xsl:template>

  <!-- ========== titles  ========== -->
  <xsl:template match="marc:datafield[@tag=130]">
    <xsl:call-template name="uniform-title"/>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=430]">
    <variant>
      <xsl:call-template name="variantTypeAttribute"/>
      <xsl:call-template name="uniform-title"/>
      <xsl:apply-templates/>
    </variant>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=530]|marc:datafield[@tag=730]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>			
      <xsl:call-template name="uniform-title"/>
      <xsl:apply-templates/>
    </related>
  </xsl:template>

  <xsl:template name="title">
    <titleInfo>
      <xsl:call-template name="setAuthority"/>
      <title>
	<xsl:variable name="str">
	  <xsl:for-each select="marc:subfield">
	    <xsl:if test="(contains('tfghklmors',@code) )">
	      <xsl:value-of select="text()"/>
	      <xsl:text> </xsl:text>
	    </xsl:if>
	  </xsl:for-each>
	</xsl:variable>

	<xsl:call-template name="chopPunctuation">
	  <xsl:with-param name="chopString">
	    <xsl:value-of select="substring($str,1,string-length($str)-1)"/>
	  </xsl:with-param>
	</xsl:call-template>
      </title>
      <xsl:call-template name="part"/>
      <!-- <xsl:call-template name="uri"/> -->
    </titleInfo>
  </xsl:template>

  <xsl:template name="uniform-title">
    <titleInfo>
      <xsl:call-template name="setAuthority"/>
      <title>
	<xsl:variable name="str">
	  <xsl:for-each select="marc:subfield">
	    <xsl:if test="(contains('adfghklmors',@code) )">
	      <xsl:value-of select="text()"/>
	      <xsl:text> </xsl:text>
	    </xsl:if>
	  </xsl:for-each>
	</xsl:variable>

	<xsl:call-template name="chopPunctuation">
	  <xsl:with-param name="chopString">
	    <xsl:value-of select="substring($str,1,string-length($str)-1)"/>
	  </xsl:with-param>
	</xsl:call-template>
      </title>
      <xsl:call-template name="part"/>
      <!-- <xsl:call-template name="uri"/> -->
    </titleInfo>
  </xsl:template>


  <!-- ========== topics  ========== -->
  <xsl:template match="marc:subfield[@code='x']">		
    <topic>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:value-of select="."/>
	</xsl:with-param>
      </xsl:call-template>
    </topic>
  </xsl:template>
  
  <xsl:template match="marc:datafield[@tag=150][marc:subfield[@code='a' or @code='b']]|marc:datafield[@tag=180][marc:subfield[@code='x']]">
    <xsl:call-template name="topic"/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=450][marc:subfield[@code='a' or @code='b']]|marc:datafield[@tag=480][marc:subfield[@code='x']]">
    <variant>
      <xsl:call-template name="variantTypeAttribute"/>
      <xsl:call-template name="topic"/>
    </variant>
  </xsl:template>
  
  <xsl:template match="marc:datafield[@tag=550 or @tag=750][marc:subfield[@code='a' or @code='b']]">
    <related>			
      <xsl:call-template name="relatedTypeAttribute"/>
      <!-- <xsl:call-template name="uri"/> -->
      <xsl:call-template name="topic"/>
    </related>
  </xsl:template> 

  <xsl:template name="topic">
    <topic>				
      <xsl:if test="@tag=550 or @tag=750">
	<xsl:call-template name="subfieldSelect">
	  <xsl:with-param name="codes">ab</xsl:with-param>
	</xsl:call-template>
      </xsl:if>
      <xsl:call-template name="setAuthority"/>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:choose>
	    <xsl:when test="@tag=180 or @tag=480 or @tag=580 or @tag=780">
	      <xsl:apply-templates select="marc:subfield[@code='x']"/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:call-template name="subfieldSelect">
		<xsl:with-param name="codes">ab</xsl:with-param>
	      </xsl:call-template>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:with-param>
      </xsl:call-template>
    </topic>
    <xsl:apply-templates/>
  </xsl:template>

  <!-- ========= temporals  ========== -->

  <xsl:template match="marc:subfield[@code='y']">
    <temporal>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:value-of select="."/>
	</xsl:with-param>
      </xsl:call-template>
    </temporal>
  </xsl:template>
  
  <xsl:template match="marc:datafield[@tag=148][marc:subfield[@code='a']]|marc:datafield[@tag=182 ][marc:subfield[@code='y']]">
    <xsl:call-template name="temporal"/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=448][marc:subfield[@code='a']]|marc:datafield[@tag=482][marc:subfield[@code='y']]">
    <variant>
      <xsl:call-template name="variantTypeAttribute"/>
      <xsl:call-template name="temporal"/>
    </variant>
  </xsl:template>
  
  <xsl:template match="marc:datafield[@tag=548 or @tag=748][marc:subfield[@code='a']]|marc:datafield[@tag=582 or @tag=782][marc:subfield[@code='y']]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <!-- <xsl:call-template name="uri"/> -->
      <xsl:call-template name="temporal"/>
    </related>
  </xsl:template>
  
  <xsl:template name="temporal">
    <temporal>
      <xsl:if test="@tag=548 or @tag=748">
	<xsl:value-of select="marc:subfield[@code='a']"/>
      </xsl:if>
      <xsl:call-template name="setAuthority"/>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:choose>
	    <xsl:when test="@tag=182 or @tag=482 or @tag=582 or @tag=782">
	      <xsl:apply-templates select="marc:subfield[@code='y']"/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="marc:subfield[@code='a']"/>
	    </xsl:otherwise>
	  </xsl:choose>	
	</xsl:with-param>
      </xsl:call-template>
    </temporal>
    <xsl:apply-templates/>
  </xsl:template>

  <!-- ========== genre  ========== -->
  <xsl:template match="marc:subfield[@code='v']">
    <genre>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:value-of select="."/>
	</xsl:with-param>
      </xsl:call-template>
    </genre>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=155][marc:subfield[@code='a']]|marc:datafield[@tag=185][marc:subfield[@code='v']]">
    <xsl:call-template name="genre"/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=455][marc:subfield[@code='a']]|marc:datafield[@tag=485 ][marc:subfield[@code='v']]">
    <variant>
      <xsl:call-template name="variantTypeAttribute"/>
      <xsl:call-template name="genre"/>
    </variant>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=555]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <!-- <xsl:call-template name="uri"/> -->
      <xsl:call-template name="genre"/>
    </related>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=555 or @tag=755][marc:subfield[@code='a']]|marc:datafield[@tag=585][marc:subfield[@code='v']]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <xsl:call-template name="genre"/>
    </related>
  </xsl:template>

  <xsl:template name="genre">
    <genre>
      <xsl:if test="@tag=555">
	<xsl:value-of select="marc:subfield[@code='a']"/>
      </xsl:if>
      <xsl:call-template name="setAuthority"/>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:choose>
	    <xsl:when test="@tag=185 or @tag=485 or @tag=585">
	      <xsl:apply-templates select="marc:subfield[@code='v']"/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="marc:subfield[@code='a']"/>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:with-param>
      </xsl:call-template>					
    </genre>
    <xsl:apply-templates/>
  </xsl:template> 
  
  <!-- ========= geographic  ========== -->
  <xsl:template match="marc:subfield[@code='z']">
    <geographic>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:value-of select="."/>
	</xsl:with-param>
      </xsl:call-template>
    </geographic>
  </xsl:template>

  <xsl:template name="geographic">		
    <geographic>					
      <xsl:if test="@tag=551">
	<xsl:value-of select="marc:subfield[@code='a']"/>
      </xsl:if> 
      <xsl:call-template name="setAuthority"/>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:choose>
	    <xsl:when test="@tag=181 or @tag=481 or @tag=581">
	      <xsl:apply-templates select="marc:subfield[@code='z']"/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="marc:subfield[@code='a']"/>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:with-param>
      </xsl:call-template>						
    </geographic>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=151][marc:subfield[@code='a']]|marc:datafield[@tag=181][marc:subfield[@code='z']]">
    <xsl:call-template name="geographic"/>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=451][marc:subfield[@code='a']]|marc:datafield[@tag=481][marc:subfield[@code='z']]">
    <variant>
      <xsl:call-template name="variantTypeAttribute"/>
      <xsl:call-template name="geographic"/>			
    </variant>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=551]|marc:datafield[@tag=581][marc:subfield[@code='z']]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <!-- <xsl:call-template name="uri"/> -->
      <xsl:call-template name="geographic"/>
    </related>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=580]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <xsl:apply-templates/>
    </related>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=751][marc:subfield[@code='z']]|marc:datafield[@tag=781][marc:subfield[@code='z']]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <xsl:call-template name="geographic"/>	
    </related>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=755]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <xsl:call-template name="genre"/>	
      <xsl:call-template name="setAuthority"/>
      <xsl:apply-templates/>
    </related>
  </xsl:template> 

  <xsl:template match="marc:datafield[@tag=780]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <xsl:apply-templates/>
    </related>
  </xsl:template>

  <xsl:template match="marc:datafield[@tag=785]">
    <related>
      <xsl:call-template name="relatedTypeAttribute"/>
      <xsl:apply-templates/>
    </related>
  </xsl:template>

  <!-- ========== notes  ========== -->
  <xsl:template match="marc:datafield[667 &lt;= @tag and @tag &lt;= 688]">
    <note>
      <xsl:choose>
	<xsl:when test="@tag=667">
	  <xsl:attribute name="type">nonpublic</xsl:attribute>
	</xsl:when>
	<xsl:when test="@tag=670">
	  <xsl:attribute name="type">source</xsl:attribute>
	</xsl:when>
	<xsl:when test="@tag=675">
	  <xsl:attribute name="type">notFound</xsl:attribute>
	</xsl:when>
	<xsl:when test="@tag=678">
	  <xsl:attribute name="type">history</xsl:attribute>
	</xsl:when>
	<xsl:when test="@tag=681">
	  <xsl:attribute name="type">subject example</xsl:attribute>
	</xsl:when>
	<xsl:when test="@tag=682">
	  <xsl:attribute name="type">deleted heading information</xsl:attribute>
	</xsl:when>
	<xsl:when test="@tag=688">
	  <xsl:attribute name="type">application history</xsl:attribute>
	</xsl:when>
      </xsl:choose>
      <xsl:call-template name="chopPunctuation">
	<xsl:with-param name="chopString">
	  <xsl:choose>
	    <xsl:when test="@tag=667 or @tag=675">
	      <xsl:value-of select="marc:subfield[@code='a']"/>
	    </xsl:when>
	    <xsl:when test="@tag=670 or @tag=678">
	      <xsl:call-template name="subfieldSelect">
		<xsl:with-param name="codes">ab</xsl:with-param>
	      </xsl:call-template>
	    </xsl:when>
	    <xsl:when test="680 &lt;= @tag and @tag &lt;=688">
	      <xsl:call-template name="subfieldSelect">
		<xsl:with-param name="codes">ai</xsl:with-param>
	      </xsl:call-template>
	    </xsl:when>
	  </xsl:choose>
	</xsl:with-param>
      </xsl:call-template>
    </note>
  </xsl:template>
  <!-- ========== url  ========== -->
  <xsl:template match="marc:datafield[@tag=856][marc:subfield[@code='u']]">
    <url>
      <xsl:if test="marc:subfield[@code='z' or @code='3']">
	<xsl:attribute name="displayLabel">
	  <xsl:call-template name="subfieldSelect">
	    <xsl:with-param name="codes">z3</xsl:with-param>
	  </xsl:call-template>
	</xsl:attribute>
      </xsl:if>
      <xsl:value-of select="marc:subfield[@code='u']"/>
    </url>
  </xsl:template>

  <xsl:template name="relatedTypeAttribute">
    <xsl:choose>
      <xsl:when test="@tag=500 or @tag=510 or @tag=511 or @tag=548 or @tag=550 or @tag=551 or @tag=555 or @tag=580 or @tag=581 or @tag=582 or @tag=585">
	<xsl:if test="substring(marc:subfield[@code='w'],1,1)='a'">
	  <xsl:attribute name="type">earlier</xsl:attribute>
	</xsl:if>
	<xsl:if test="substring(marc:subfield[@code='w'],1,1)='b'">
	  <xsl:attribute name="type">later</xsl:attribute>
	</xsl:if>
	<xsl:if test="substring(marc:subfield[@code='w'],1,1)='t'">
	  <xsl:attribute name="type">parentOrg</xsl:attribute>
	</xsl:if>
	<xsl:if test="substring(marc:subfield[@code='w'],1,1)='g'">
	  <xsl:attribute name="type">broader</xsl:attribute>
	</xsl:if>
	<xsl:if test="substring(marc:subfield[@code='w'],1,1)='h'">
	  <xsl:attribute name="type">narrower</xsl:attribute>
	</xsl:if>
	<xsl:if test="contains('fin|', substring(marc:subfield[@code='w'],1,1))">
	  <xsl:attribute name="type">other</xsl:attribute>
	</xsl:if>
      </xsl:when>
      <xsl:when test="@tag=530 or @tag=730">
	<xsl:attribute name="type">other</xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
	<!-- 7xx -->
	<xsl:attribute name="type">equivalent</xsl:attribute>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="marc:subfield[@code='i']"/>
  </xsl:template>

  <xsl:template name="variantTypeAttribute">
    <xsl:choose>
      <xsl:when test="@tag=400 or @tag=410 or @tag=411 or @tag=451 or @tag=455 or @tag=480 or @tag=481 or @tag=482 or @tag=485">
	<xsl:if test="substring(marc:subfield[@code='w'],1,1)='d'">
	  <xsl:attribute name="type">acronym</xsl:attribute>
	</xsl:if>
	<xsl:if test="contains('fit', substring(marc:subfield[@code='w'],1,1))">
	  <xsl:attribute name="type">other</xsl:attribute>
	</xsl:if>
      </xsl:when>
      <xsl:otherwise>
	<!-- 430  -->
	<xsl:attribute name="type">other</xsl:attribute>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="marc:subfield[@code='i']"/>
  </xsl:template>
  <xsl:template name="setAuthority">
    <xsl:choose>
      <!-- can be called from the datafield or subfield level, so "..//@tag" means
	   the tag can be at the subfield's parent level or at the datafields own level -->
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=100 and (@ind1=0 or @ind1=1) and $controlField008-11='a' and $controlField008-14='a'">   
	<xsl:attribute name="authority"><xsl:text>naf</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=100 and (@ind1=0 or @ind1=1) and $controlField008-11='a' and $controlField008-14='b'">   
	<xsl:attribute name="authority"><xsl:text>lcsh</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=100 and (@ind1=0 or @ind1=1) and $controlField008-11='k'">   
	<xsl:attribute name="authority"><xsl:text>lacnaf</xsl:text></xsl:attribute>
      </xsl:when>   
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=100 and @ind1=3 and $controlField008-11='a' and $controlField008-14='b'">
	<xsl:attribute name="authority"><xsl:text>lcsh</xsl:text></xsl:attribute>
      </xsl:when>   
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=100 and @ind1=3 and $controlField008-11='k' and $controlField008-14='b'">
	<xsl:attribute name="authority">cash</xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=110 and $controlField008-11='a' and $controlField008-14='a'">
	<xsl:attribute name="authority">naf</xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=110 and $controlField008-11='a' and $controlField008-14='b'">
	<xsl:attribute name="authority">lcsh</xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=110 and $controlField008-11='k' and $controlField008-14='a'">
	<xsl:attribute name="authority"><xsl:text>lacnaf</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=110 and $controlField008-11='k' and $controlField008-14='b'">
	<xsl:attribute name="authority"><xsl:text>cash</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="100 &lt;= ancestor-or-self::marc:datafield/@tag and ancestor-or-self::marc:datafield/@tag &lt;= 155 and $controlField008-11='b'">
	<xsl:attribute name="authority"><xsl:text>lcshcl</xsl:text></xsl:attribute>
      </xsl:when>   
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=100 or ancestor-or-self::marc:datafield/@tag=110 or ancestor-or-self::marc:datafield/@tag=111 or ancestor-or-self::marc:datafield/@tag=130 or ancestor-or-self::marc:datafield/@tag=151) and $controlField008-11='c'">
	<xsl:attribute name="authority"><xsl:text>nlmnaf</xsl:text></xsl:attribute>
      </xsl:when>   
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=100 or ancestor-or-self::marc:datafield/@tag=110 or ancestor-or-self::marc:datafield/@tag=111 or ancestor-or-self::marc:datafield/@tag=130 or ancestor-or-self::marc:datafield/@tag=151) and $controlField008-11='d'">   
	<xsl:attribute name="authority"><xsl:text>nalnaf</xsl:text></xsl:attribute>
      </xsl:when>   
      <xsl:when test="100 &lt;= ancestor-or-self::marc:datafield/@tag and ancestor-or-self::marc:datafield/@tag &lt;= 155 and $controlField008-11='r'">
	<xsl:attribute name="authority"><xsl:text>aat</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="100 &lt;= ancestor-or-self::marc:datafield/@tag and ancestor-or-self::marc:datafield/@tag &lt;= 155 and $controlField008-11='s'">
	<xsl:attribute name="authority">sears</xsl:attribute>
      </xsl:when>
      <xsl:when test="100 &lt;= ancestor-or-self::marc:datafield/@tag and ancestor-or-self::marc:datafield/@tag &lt;= 155 and $controlField008-11='v'">
	<xsl:attribute name="authority">rvm</xsl:attribute>
      </xsl:when>
      <xsl:when test="100 &lt;= ancestor-or-self::marc:datafield/@tag and ancestor-or-self::marc:datafield/@tag &lt;= 155 and $controlField008-11='z'">
	<xsl:attribute name="authority"><xsl:value-of select="../marc:datafield[ancestor-or-self::marc:datafield/@tag=040]/marc:subfield[@code='f']"/></xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=111 or ancestor-or-self::marc:datafield/@tag=130) and $controlField008-11='a' and $controlField008-14='a'">
	<xsl:attribute name="authority"><xsl:text>naf</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=111 or ancestor-or-self::marc:datafield/@tag=130) and $controlField008-11='a' and $controlField008-14='b'">
	<xsl:attribute name="authority"><xsl:text>lcsh</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=111 or ancestor-or-self::marc:datafield/@tag=130) and $controlField008-11='k' ">
	<xsl:attribute name="authority"><xsl:text>lacnaf</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=148 or ancestor-or-self::marc:datafield/@tag=150  or ancestor-or-self::marc:datafield/@tag=155) and $controlField008-11='a' ">
	<xsl:attribute name="authority"><xsl:text>lcsh</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=148 or ancestor-or-self::marc:datafield/@tag=150  or ancestor-or-self::marc:datafield/@tag=155) and $controlField008-11='a' ">
	<xsl:attribute name="authority"><xsl:text>lcsh</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=148 or ancestor-or-self::marc:datafield/@tag=150  or ancestor-or-self::marc:datafield/@tag=155) and $controlField008-11='c' ">
	<xsl:attribute name="authority"><xsl:text>mesh</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=148 or ancestor-or-self::marc:datafield/@tag=150  or ancestor-or-self::marc:datafield/@tag=155) and $controlField008-11='d' ">
	<xsl:attribute name="authority"><xsl:text>nal</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=148 or ancestor-or-self::marc:datafield/@tag=150  or ancestor-or-self::marc:datafield/@tag=155) and $controlField008-11='k' ">
	<xsl:attribute name="authority"><xsl:text>cash</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=151 and $controlField008-11='a' and $controlField008-14='a'">
	<xsl:attribute name="authority"><xsl:text>naf</xsl:text></xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=151 and $controlField008-11='a' and $controlField008-14='b'">
	<xsl:attribute name="authority">lcsh</xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=151 and $controlField008-11='k' and $controlField008-14='a'">
	<xsl:attribute name="authority">lacnaf</xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=151 and $controlField008-11='k' and $controlField008-14='b'">
	<xsl:attribute name="authority">cash</xsl:attribute>
      </xsl:when>
      
      <xsl:when test="(..//ancestor-or-self::marc:datafield/@tag=180 or ..//ancestor-or-self::marc:datafield/@tag=181 or ..//ancestor-or-self::marc:datafield/@tag=182 or ..//ancestor-or-self::marc:datafield/@tag=185) and $controlField008-11='a'">
	<xsl:attribute name="authority">lcsh</xsl:attribute>
      </xsl:when>
      
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=700 and (@ind1='0' or @ind1='1') and @ind2='0'">   
	<xsl:attribute name="authority">naf</xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=700 and (@ind1='0' or @ind1='1') and @ind2='5'">   
	<xsl:attribute name="authority">lacnaf</xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=700 and @ind1='3' and @ind2='0'">   
	<xsl:attribute name="authority">lcsh</xsl:attribute>
      </xsl:when>
      <xsl:when test="ancestor-or-self::marc:datafield/@tag=700 and @ind1='3' and @ind2='5'">   
	<xsl:attribute name="authority">cash</xsl:attribute>
      </xsl:when>
      <xsl:when test="(700 &lt;= ancestor-or-self::marc:datafield/@tag and ancestor-or-self::marc:datafield/@tag &lt;= 755 ) and @ind2='1'">   
	<xsl:attribute name="authority">lcshcl</xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=700 or ancestor-or-self::marc:datafield/@tag=710 or ancestor-or-self::marc:datafield/@tag=711 or ancestor-or-self::marc:datafield/@tag=730 or ancestor-or-self::marc:datafield/@tag=751)  and @ind2='2'">   
	<xsl:attribute name="authority">nlmnaf</xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=700 or ancestor-or-self::marc:datafield/@tag=710 or ancestor-or-self::marc:datafield/@tag=711 or ancestor-or-self::marc:datafield/@tag=730 or ancestor-or-self::marc:datafield/@tag=751)  and @ind2='3'">   
	<xsl:attribute name="authority">nalnaf</xsl:attribute>
      </xsl:when>   
      <xsl:when test="(700 &lt;= ancestor-or-self::marc:datafield/@tag and ancestor-or-self::marc:datafield/@tag &lt;= 755 ) and @ind2='6'">
	<xsl:attribute name="authority">rvm</xsl:attribute>
      </xsl:when>
      <xsl:when test="(700 &lt;= ancestor-or-self::marc:datafield/@tag and ancestor-or-self::marc:datafield/@tag &lt;= 755 ) and @ind2='7'">   
	<xsl:attribute name="authority">
	  <xsl:value-of select="marc:subfield[@code='2']"/>
	</xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=710 or ancestor-or-self::marc:datafield/@tag=711 or ancestor-or-self::marc:datafield/@tag=730 or ancestor-or-self::marc:datafield/@tag=751)  and @ind2='5'">   
	<xsl:attribute name="authority">lacnaf</xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=710 or ancestor-or-self::marc:datafield/@tag=711 or ancestor-or-self::marc:datafield/@tag=730 or ancestor-or-self::marc:datafield/@tag=751)  and @ind2='0'">   
	<xsl:attribute name="authority">naf</xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=748 or ancestor-or-self::marc:datafield/@tag=750 or ancestor-or-self::marc:datafield/@tag=755)  and @ind2='0'">  
	<xsl:attribute name="authority">lcsh</xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=748 or ancestor-or-self::marc:datafield/@tag=750 or ancestor-or-self::marc:datafield/@tag=755)  and @ind2='2'">  
	<xsl:attribute name="authority">mesh</xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=748 or ancestor-or-self::marc:datafield/@tag=750 or ancestor-or-self::marc:datafield/@tag=755)  and @ind2='3'">      
	<xsl:attribute name="authority">nal</xsl:attribute>
      </xsl:when>
      <xsl:when test="(ancestor-or-self::marc:datafield/@tag=748 or ancestor-or-self::marc:datafield/@tag=750 or ancestor-or-self::marc:datafield/@tag=755)  and @ind2='5'">      
	<xsl:attribute name="authority">cash</xsl:attribute>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!--<xsl:template name="oldsetAuth">
      <xsl:param name="ind2"/>
      <xsl:if test="100 &lt;= @tag and @tag &lt; 200">
      <xsl:attribute name="authority">
      <xsl:value-of select="$auth"/>
      </xsl:attribute>
      </xsl:if>
      <xsl:if test="(700 &lt;= @tag and @tag &lt; 800) or /marc:datafield[700 &lt;=@tag and @tag &lt; 800]">
      <xsl:attribute name="authority">
      <xsl:choose>
      <xsl:when test="$ind2='0'">naf</xsl:when>					
      <xsl:when test="$ind2='2'">nlm</xsl:when>
      <xsl:when test="$ind2='3'">nal</xsl:when>
      <xsl:when test="$ind2='5'">lac</xsl:when>
      <xsl:when test="$ind2='6'">rvm</xsl:when>					
      <xsl:when test="$ind2='7'"><xsl:value-of select="../marc:subfield[@code='2']"/></xsl:when>									
      </xsl:choose>
      </xsl:attribute>
      </xsl:if>
      </xsl:template>
  -->
  <!--<xsl:template name="oldsetAuth2">
      <xsl:param name="ind2"/>
      <xsl:if test="(100 &lt;= @tag and @tag &lt; 200) or ../marc:datafield[100 &lt;=@tag and @tag &lt; 200]">
      <xsl:attribute name="authority">
      <xsl:choose>
      <xsl:when test="$controlField008-11='a'">lcsh</xsl:when>
      <xsl:when test="$controlField008-11='b'">lcac</xsl:when>
      <xsl:when test="$controlField008-11='c'">mesh</xsl:when>
      <xsl:when test="$controlField008-11='d'">nal</xsl:when>
      <xsl:when test="$controlField008-11='k'">cash</xsl:when>
      <xsl:when test="$controlField008-11='r'">aat</xsl:when>
      <xsl:when test="$controlField008-11='s'">sears</xsl:when>
      <xsl:when test="$controlField008-11='v'">rvm</xsl:when>
      <xsl:when test="$controlField008-11='z'"><xsl:value-of select="../marc:datafield[@tag=040]/marc:subfield[@code='f']"/></xsl:when>				
      </xsl:choose>
      </xsl:attribute>
      </xsl:if>
      <xsl:if test="(500 &lt;= @tag and @tag &lt; 800) or /marc:datafield[500 &lt;=@tag and @tag &lt; 800]">
      <xsl:attribute name="authority">
      <xsl:choose>
      <xsl:when test="$ind2='0'">lcsh</xsl:when>
      <xsl:when test="$ind2='1'">lcac</xsl:when>
      <xsl:when test="$ind2='2'">mesh</xsl:when>
      <xsl:when test="$ind2='3'">nal</xsl:when>
      <xsl:when test="$ind2='5'">cash</xsl:when>
      <xsl:when test="$ind2='6'">rvm</xsl:when>					
      <xsl:when test="$ind2='7'"><xsl:value-of select="../marc:subfield[@code='2']"/></xsl:when>									
      </xsl:choose>
      </xsl:attribute>
      </xsl:if>
      </xsl:template>-->
  <!--<xsl:template match="marc:subfield[@code='0']">
      <xsl:attribute name="xlink:href">
      <xsl:value-of select="."/>
      </xsl:attribute>
      </xsl:template>
  -->
  <xsl:template match="*"/>
  <!--<xsl:template match="*">
      <n><xsl:attribute name="tag"><xsl:value-of select="@tag"/></xsl:attribute>
      <xsl:attribute name="code"><xsl:value-of select="@code"/></xsl:attribute>
      <xsl:value-of select="name()"/>
      
      <v><xsl:value-of select="."/></v></n>
      </xsl:template>-->
</xsl:stylesheet><!-- Stylus Studio meta-information - (c)1998-2003 Copyright Sonic Software Corporation. All rights reserved.
		      <metaInformation>
		      <scenarios ><scenario default="yes" name="Scenario1" userelativepaths="yes" externalpreview="no" url="..\..\documents\mads_short.xml" htmlbaseurl="" outputurl="" processortype="internal" commandline="" additionalpath="" additionalclasspath="" postprocessortype="none" postprocesscommandline="" postprocessadditionalpath="" postprocessgeneratedext=""/></scenarios><MapperInfo srcSchemaPath="" srcSchemaRoot="" srcSchemaPathIsRelative="yes" srcSchemaInterpretAsXML="no" destSchemaPath="" destSchemaRoot="" destSchemaPathIsRelative="yes" destSchemaInterpretAsXML="no"/>
		      </metaInformation>
		 -->