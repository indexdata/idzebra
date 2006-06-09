<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns:z="http://indexdata.dk/zebra/xslt/1" 
    xmlns:marc="http://www.loc.gov/MARC21/slim" 
    version="1.0">

  <!-- <xsl:include href="http://www.loc.gov/marcxml/xslt/MARC21slimUtils.xsl"/>-->
  <!-- <xsl:include href="MARC21slimUtils.xsl"/> -->
  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>

  <!-- disable all default text node output -->
  <xsl:template match="text()"/>

  <xsl:template match="/">
    <xsl:if test="marc:collection">
      <collection>
         <xsl:apply-templates select="marc:collection/marc:record"/>
       </collection>
    </xsl:if>
    <xsl:if test="marc:record">
       <xsl:apply-templates select="marc:record"/>
    </xsl:if>
  </xsl:template>


  <!-- match on marcxml record -->
  <xsl:template match="marc:record">                
    <xsl:variable name="leader" select="marc:leader"/>
    <xsl:variable name="leader6" select="substring($leader,7,1)"/>
    <xsl:variable name="leader7" select="substring($leader,8,1)"/>
    <xsl:variable name="controlField001" select="marc:controlfield[@tag=001]"/>
    <xsl:variable name="controlField008" select="marc:controlfield[@tag=008]"/>

    <xsl:variable name="typeOf008">
      <xsl:choose>
        <xsl:when test="$leader6='a'">
          <xsl:choose>
            <xsl:when test="$leader7='a' or $leader7='c' or $leader7='d'
                            or $leader7='m'">BK</xsl:when>
            <xsl:when test="$leader7='b' or $leader7='i' 
                            or $leader7='s'">SE</xsl:when>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$leader6='t'">BK</xsl:when>
        <xsl:when test="$leader6='p'">MM</xsl:when>
        <xsl:when test="$leader6='m'">CF</xsl:when>
        <xsl:when test="$leader6='e' or $leader6='f'">MP</xsl:when>
        <xsl:when test="$leader6='g' or $leader6='k' or $leader6='o' 
                        or $leader6='r'">VM</xsl:when>
        <xsl:when test="$leader6='c' or $leader6='d' or $leader6='i' 
                        or $leader6='j'">MU</xsl:when>
      </xsl:choose>
    </xsl:variable>

    <z:record id="{$controlField001}" type="update">
      <!-- <xsl:attribute name="id"></xsl:attribute> -->
      <!-- <xsl:attribute name="type">update</xsl:attribute> -->
      <!-- <xsl:attribute name="rank"></xsl:attribute> -->

      <!-- calling simple templates working only on one field at a time -->
      <!-- <xsl:apply-templates select="marc:controlfield"/> -->
      <!-- <xsl:apply-templates select="marc:datafield"/> -->

      <xsl:call-template name="bib1_rules"/>

    </z:record>
  </xsl:template>

  <xsl:template name="bib1_rules">
      <!-- att 1               Personal-name -->
      <!-- att 2               Corporate-name -->
      <!-- att 3               Conference-name -->
      <!-- att 4               Title -->
      <xsl:call-template name="Title"/>
      <!-- att 5               Title-series -->
      <!-- att 6               Title-uniform -->
      <!-- att 7               ISBN -->
      <!-- att 8               ISSN -->
      <!-- att 9               LC-card-number -->
      <!-- att 10              BNB-card-number -->
      <!-- att 11              BGF-number -->
      <!-- att 12              Local-number -->
      <!-- att 13              Dewey-classification -->
      <!-- att 14              UDC-classification -->
      <!-- att 15              Bliss-classification -->
      <!-- att 16              LC-call-number -->
      <!-- att 17              NLM-call-number -->
      <!-- att 18              NAL-call-number -->
      <!-- att 19              MOS-call-number -->
      <!-- att 20              Local-classification -->
      <!-- att 21              Subject-heading -->
      <xsl:call-template name="Subject-heading"/>
      <!-- att 22              Subject-Rameau -->
      <!-- att 23              BDI-index-subject -->
      <!-- att 24              INSPEC-subject -->
      <!-- att 25              MESH-subject -->
      <!-- att 26              PA-subject -->
      <!-- att 27              LC-subject-heading -->
      <!-- att 28              RVM-subject-heading -->
      <!-- att 29              Local-subject-index -->
      <!-- att 30              Date -->
      <!-- att 31              Date-of-publication -->
      <!-- att 32              Date-of-acquisition -->
      <!-- att 33              Title-key -->
      <!-- att 34              Title-collective -->
      <!-- att 35              Title-parallel -->
      <!-- att 36              Title-cover -->
      <!-- att 37              Title-added-title-page -->
      <!-- att 38              Title-caption -->
      <!-- att 39              Title-running -->
      <!-- att 40              Title-spine -->
      <!-- att 41              Title-other-variant -->
      <!-- att 42              Title-former -->
      <!-- att 43              Title-abbreviated -->
      <!-- att 44              Title-expanded -->
      <!-- att 45              Subject-precis -->
      <!-- att 46              Subject-rswk -->
      <!-- att 47              Subject-subdivision -->
      <!-- att 48              Number-natl-biblio -->
      <!-- att 49              Number-legal-deposit -->
      <!-- att 50              Number-govt-pub -->
      <!-- att 51              Number-music-publisher -->
      <!-- att 52              Number-db -->
      <!-- att 53              Number-local-call -->
      <!-- att 54              Code-language -->
      <!-- att 55              Code-geographic -->
      <!-- att 56              Code-institution -->
      <!-- att 57              Name-and-title -->      
      <!-- att 58              Name-geographic -->
      <!-- att 59              Place-publication -->
      <!-- att 60              CODEN -->
      <!-- att 61              Microform-generation -->
      <!-- att 62              Abstract -->
      <xsl:call-template name="Abstract"/>
      <!-- att 63              Note -->
      <!-- att 1000            Author-title -->
      <xsl:call-template name="Author-title"/>
      <!-- att 1001            Record-type -->
      <!-- att 1002            Name -->
      <!-- att 1003            Author -->
      <xsl:call-template name="Author"/>
      <!-- att 1004            Author-name-personal -->
      <xsl:call-template name="Author-name-personal"/>
      <!-- att 1005            Author-name-corporate -->
      <xsl:call-template name="Author-name-corporate"/>
      <!-- att 1006            Author-name-conference -->
      <xsl:call-template name="Author-name-conference"/>
      <!-- att 1007            Identifier-standard -->
      <!-- att 1008            Subject-LC-childrens -->
      <!-- att 1009            Subject-name-personal -->
      <!-- att 1010            Body-of-text -->
      <!-- att 1011            Date/time-added-to-db -->
      <!-- att 1012            Date/time-last-modified -->
      <!-- att 1013            Authority/format-id -->
      <!-- att 1014            Concept-text -->
      <!-- att 1015            Concept-reference -->
      <!-- att 1016            Any -->
      <!-- att 1017            Server-choice -->
      <!-- att 1018            Publisher -->
      <!-- att 1019            Record-source -->
      <!-- att 1020            Editor -->
      <!-- att 1021            Bib-level -->
      <!-- att 1022            Geographic-class -->
      <!-- att 1023            Indexed-by -->
      <!-- att 1024            Map-scale -->
      <!-- att 1025            Music-key -->
      <!-- att 1026            Related-periodical -->
      <!-- att 1027            Report-number -->
      <!-- att 1028            Stock-number -->
      <!-- att 1030            Thematic-number -->
      <!-- att 1031            Material-type -->
      <!-- att 1032            Doc-id -->
      <!-- att 1033            Host-item -->
      <!-- att 1034            Content-type -->
      <!-- att 1035            Anywhere -->
      <!-- att 1036            Author-Title-Subject -->
  </xsl:template>


  <xsl:template name="Abstract">
    <xsl:for-each select="marc:datafield[@tag='520']">
      <z:index name="Abstract" type="w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="Author">
    <xsl:for-each select="marc:datafield[@tag='100']/marc:subfield[@code='a']
                          | marc:datafield[@tag='110']
                          | marc:datafield[@tag='111']">
      <z:index name="Author" type="w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
    <xsl:for-each select="marc:datafield[@tag='100']">
      <z:index name="Author" type="p">
        <xsl:value-of select="marc:subfield[@code='a']"/>
        <xsl:text> </xsl:text>
        <xsl:value-of select="marc:subfield[@code='d']"/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

  <!-- collection info from different data fields -->
  <xsl:template name="Author-title">
    <xsl:if test="marc:datafield[@tag='100']
                  and marc:datafield[@tag='245']">
      <z:index name="Author-title" type="p">
        <xsl:value-of 
            select="marc:datafield[@tag='100']/marc:subfield[@code='a']"/>
        <xsl:text> </xsl:text>
        <xsl:value-of 
            select="marc:datafield[@tag='100']/marc:subfield[@code='d']"/>
        <xsl:text> </xsl:text>
        <xsl:value-of 
            select="marc:datafield[@tag='245']/marc:subfield[@code='a']"/>
      </z:index>
    </xsl:if>
  </xsl:template>

  <xsl:template name="Author-name-personal">
    <xsl:for-each select="marc:datafield[@tag='100']">
      <xsl:for-each select="marc:subfield[@code='a']">
        <z:index name="Author-name-personal" type="w">
          <xsl:value-of select="."/>
        </z:index>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:for-each select="marc:datafield[@tag='100']">
      <z:index name="Author-name-personal" type="p">
        <xsl:value-of select="marc:subfield[@code='a']"/>
        <xsl:text> </xsl:text>
        <xsl:value-of select="marc:subfield[@code='d']"/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="Author-name-corporate">
    <xsl:for-each select="marc:datafield[@tag='110']">
      <z:index name="Author-name-corporate" type="w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="Author-name-conference">
    <xsl:for-each select="marc:datafield[@tag='111']">
      <z:index name="Author-name-conference" type="w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>


  <xsl:template name="Subject-heading">
    <xsl:for-each select="marc:datafield[@tag='600']
                          |marc:datafield[@tag='610']
                          |marc:datafield[@tag='611']
                          |marc:datafield[@tag='630']
                          |marc:datafield[@tag='650']
                          |marc:datafield[@tag='651']
                          |marc:datafield[@tag='653']
                          |marc:datafield[@tag='654']
                          |marc:datafield[@tag='655']
                          |marc:datafield[@tag='656']
                          |marc:datafield[@tag='657']">
      <z:index name="Subject-heading" type="w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
    <xsl:for-each select="marc:datafield[@tag='600']
                          |marc:datafield[@tag='650']
                          |marc:datafield[@tag='651']
                          |marc:datafield[@tag='653']">
      <z:index name="Subject-heading" type="w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="Title">
    <xsl:for-each select="marc:datafield[@tag='245']/marc:subfield[@code='a']">
      <z:index name="Title" type="w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>



  <!--
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
      <xsl:value-of select="marc:subfield[@code='a']"/>
    </identifier>
  </xsl:template>


  <xsl:template match="marc:datafield[@tag=024]">
    <identifier>
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
      <name type="conference">
        <xsl:call-template name="setAuthority"/>
        <xsl:call-template name="nameACDENQ"/>
      </name>
      <xsl:apply-templates/>
    </related>
  </xsl:template>
  -->
 
   <!-- title processing -->

  <!--
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
    </titleInfo>
  </xsl:template>
   -->



</xsl:stylesheet>
