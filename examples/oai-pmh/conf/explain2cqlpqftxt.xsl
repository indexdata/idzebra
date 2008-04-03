<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:e="http://explain.z3950.org/dtd/2.0/"
                version="1.0">
  
  <xsl:output method="text" encoding="UTF-8"/>
  
  <!-- disable all default text node output -->
  <xsl:template match="text()"/>
  
  <!-- match zeerex xml record -->
  <xsl:template match="e:explain">
    <xsl:text>
# Propeties file to drive org.z3950.zing.cql.CQLNode's toPQF()
# back-end and the YAZ CQL-to-PQF converter.  This specifies the
# interpretation of various CQL indexes, relations, etc. in terms
# of Type-1 query attributes.
#
# This file is created from a valid ZeeRex Explain XML record using the 
# XSLT transformation 'explain2cqlpqftxt.xsl'
#
# xsltproc explain2cqlpqf.xsl explain.xml
    </xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>    
    <xsl:apply-templates/>

    <xsl:call-template name="static-text"/>

  </xsl:template>
  
  <xsl:template match="e:databaseInfo">
    <xsl:text># Title: </xsl:text><xsl:value-of select="e:title"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>
  

  <xsl:template match="e:indexInfo">
    <xsl:variable name="set-default">
      <xsl:value-of select="../e:configInfo/e:default[@type='contextSet']"/>
    </xsl:variable>

    <xsl:text># Set info</xsl:text>
    <xsl:text>&#10;</xsl:text>

    <xsl:if test="$set-default">
      <xsl:text># Default set</xsl:text>
      <xsl:text>&#10;</xsl:text>       
       <xsl:text>set = </xsl:text>
       <xsl:value-of select="e:set[@name = $set-default]/@identifier"/>
       <xsl:text>&#10;</xsl:text>
      <xsl:text>&#10;</xsl:text>       
    </xsl:if>

    <xsl:for-each select="e:set">
      <!--
      <xsl:text># </xsl:text>
      <xsl:value-of select="e:title"/>
      <xsl:text>&#10;</xsl:text>
      -->
      <xsl:text>set.</xsl:text><xsl:value-of select="@name"/>
      <xsl:text> = </xsl:text><xsl:value-of select="@identifier"/>
      <xsl:text>&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>&#10;</xsl:text>

    <xsl:text># Index info</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:for-each select="e:index">
      <!--
      <xsl:text># </xsl:text><xsl:value-of select="e:title"/>
      <xsl:text>&#10;</xsl:text>
      -->
      <xsl:text>index.</xsl:text>
      <xsl:value-of select="e:map/e:name/@set"/>
      <xsl:text>.</xsl:text>
      <xsl:value-of select="e:map/e:name/text()"/>
      <xsl:text> = </xsl:text>
      <xsl:for-each select="e:map/e:attr">
        <xsl:value-of select="@type"/>
        <xsl:text>=</xsl:text>
        <xsl:value-of select="text()"/>
        <xsl:text> </xsl:text>
      </xsl:for-each>
      <xsl:text>&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="e:configInfo">
    <xsl:text># Relation info</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:for-each select="e:supports[@type='relation']">
      <xsl:choose>
        <xsl:when test="text()='&lt;'">
          <xsl:text>relation.&lt;  = 2=1</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='le'">
          <xsl:text>relation.le     = 2=2</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='='">
          <xsl:text>relation.eq  = 2=3</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='eq'">
          <xsl:text>relation.eq      = 2=3</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='exact'">
          <xsl:text>relation.exact = 2=3</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='ge'">
          <xsl:text>relation.ge  = 2=4</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='&gt;'">
          <xsl:text>relation.&gt;  = 2=5</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='&lt;&gt;'">
          <xsl:text>relation.&lt;&gt; = 2=6</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='all'">
          <xsl:text>relation.all      = 2=3</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='any'">
          <xsl:text>relation.any      = 2=3</xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>

    <xsl:text># Default Relation</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>relation.scr = 2=3</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>

    <xsl:text># RelationModifier info</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:for-each select="e:supports[@type='relationModifier']">
      <xsl:choose>
        <xsl:when test="text()='relevant'">
          <xsl:text>relationModifier.relevant = 2=102 </xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='fuzzy'">
          <xsl:text>relationModifier.fuzzy = 2=100 </xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='stem'">
          <xsl:text>relationModifier.stem = 2=101 </xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='phonetic'">
          <xsl:text>relationModifier.phonetic  = 2=100 </xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="text()='phrase'">
          <xsl:text>relationModifier.phrase = 6=3 </xsl:text>
          <xsl:text>&#10;</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:for-each>
    
  </xsl:template>

<xsl:template name="static-text">
    <xsl:text>

# Position attributes 
position.first                          = 3=1 6=1
position.any                            = 3=3 6=1
position.last                           = 3=4 6=1
position.firstAndLast                   = 3=3 6=3

# Structure attributes may be specified for individual relations; a
# default structure attribute my be specified by the pseudo-relation
# "*", to be used whenever a relation not listed here occurs.
#
structure.exact                         = 4=108
structure.all                           = 4=2
structure.any                           = 4=2
structure.*                             = 4=1
structure.eq                            = 4=3

# Truncation attributes used to implement CQL wildcard patterns.  The
# simpler forms, left, right- and both-truncation will be used for the
# simplest patterns, so that we produce PQF queries that conform more
# closely to the Bath Profile.  However, when a more complex pattern
# such as "foo*bar" is used, we fall back on Z39.58-style masking.
#
truncation.right                        = 5=1
truncation.left                         = 5=2
truncation.both                         = 5=3
truncation.none                         = 5=100
truncation.z3958                        = 5=104

# Finally, any additional attributes that should always be included
# with each term can be specified in the "always" property.
#
# always                                        = 6=1
# 6=1: completeness = incomplete subfield
    </xsl:text>
  </xsl:template>


</xsl:stylesheet>
