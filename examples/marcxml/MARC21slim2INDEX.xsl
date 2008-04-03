<?xml version="1.0" encoding="UTF-8"?>
<!-- 
   Copyright (C) 1995-2006
   Index Data ApS

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
-->

<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns:z="http://indexdata.com/zebra-2.0" 
    xmlns:marc="http://www.loc.gov/MARC21/slim" 
    version="1.0">

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
    <xsl:variable name="controlField001" 
                  select="normalize-space(marc:controlfield[@tag='001'])"/>
    <xsl:variable name="controlField008" 
                  select="normalize-space(marc:controlfield[@tag='008'])"/>

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

     <z:record z:id="{$controlField001}" type="update">


       <!-- <xsl:attribute name="id"></xsl:attribute> -->
       <!-- <xsl:attribute name="type">update</xsl:attribute> -->
       <!-- <xsl:attribute name="rank"></xsl:attribute> -->

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
       <xsl:call-template name="ISBN"/>
       <!-- att 8               ISSN -->
       <xsl:call-template name="ISSN"/>
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

   <!-- ATTRIBUTE SET BIB-1 (Z39.50-1995) SEMANTICS -->
   <!-- TABLE 2:  USE ATTRIBUTES (CLASSIFIED AND DEFINED) -->
   <!-- Use   Value  Definition  USMARC tag(s) -->

   <!--
       Abstract               62  An abbreviated, accurate        520
                                  representation of a work,
                                  usually without added
                                 interpretation or criticism.
   -->
   <xsl:template name="Abstract">
     <xsl:for-each select="marc:datafield[@tag='520']">
       <z:index name="Abstract:w">
         <xsl:value-of select="."/>
       </z:index>
     </xsl:for-each>
   </xsl:template>
   
   <!--
       Any                  1016  The record is selected if there
                                  exists a Use attribute that the
                                  target supports (and considers
                                  appropriate - see note 1) such
                                  that the record would be
                                  selected if the target were to
                                  substitute that attribute.
       Notes:
        (1) When the origin uses 'any' the intent is that the target
            locate records via commonly used access points. The target
            may define 'any' to refer to a selected set of Use
            attributes corresponding to its commonly used access points.
        (2) In set terminology: when Any is the Use attribute, the set
            of records selected is the union of the sets of records
            selected by each of the (appropriate) Use attributes that
            the target supports.

   -->

   <!--
       Anywhere             1035  The record is selected if the
                                  term value (as qualified by the
                                  other attributes) occurs anywhere
                                 in the record.

            Note: A target might choose to support 'Anywhere' only in
            combination with specific (non-Use) attributes. For example, a
            target might support 'Anywhere' only in combination with the
            Relation attribute 'AlwaysMatches' (see below), to locate all
            records in a database.

       Notes on relationship of Any and Anywhere:
        (1) A target may support Any but not Anywhere, or vice versa, or
            both.  However, if a target supports both, then it should
            exclude 'Anywhere' from the list of Use attributes
            corresponding to 'Any' (if it does not do so, then the set
            of records located by 'Any' will be a superset of those
            located by 'Anywhere').
        (2) A distinction between the two attributes may be informally
            expressed as follows: 'anywhere' might result in more
            expensive searching than 'any'; if the target (and origin)
            support both 'any' and 'anywhere', if the origin uses 'Any'
            (rather than 'Anywhere') it is asking the target to locate
            the term only if it can do so relatively inexpensively.

   -->

   <!--
       Author-name  1003  A personal or corporate author, 100, 110, 111, 400
                           or a conference or meeting      410, 411, 700, 710,
                           name.  (No subject name         711, 800, 810, 811
                           headings are included.)

   -->
  <xsl:template name="Author">
    <xsl:for-each select="marc:datafield[@tag='100']/marc:subfield[@code='a']
                          | marc:datafield[@tag='110']
                          | marc:datafield[@tag='111']
                          | marc:datafield[@tag='400']
                          | marc:datafield[@tag='410']
                          | marc:datafield[@tag='700']
                          | marc:datafield[@tag='710']
                          | marc:datafield[@tag='711']
                          | marc:datafield[@tag='800']
                          | marc:datafield[@tag='810']
                          | marc:datafield[@tag='811']">
      <z:index name="Author:w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
    <xsl:for-each select="marc:datafield[@tag='100']">
      <z:index name="Author:p">
        <xsl:value-of select="marc:subfield[@code='a']"/>
        <xsl:text> </xsl:text>
        <xsl:value-of select="marc:subfield[@code='d']"/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

   <!--
Author-name-and-     1000  A personal or corporate author, 100/2XX, 110/2XX,
title                      or a conference or meeting      111/2XX, subfields
                           name, and the title of the      $a & $t in
                           item.  (No subject name         following: 400,410,
                           headings are included.)  The    411, 700, 710, 711,
                           syntax of the name-title        800, 810, 811
                           combination is up to the
                           target, unless used with the
                           Structure attribute Key (see
                           below).

   -->
  <xsl:template name="Author-title">
    <xsl:if test="marc:datafield[@tag='100']
                  and marc:datafield[@tag='245']">
      <z:index name="Author-title:p">
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


   <!--
Author-name-         1005  An organization or a group      110, 410, 710, 810
corporate                  of persons that is identified
                           by a particular name. (Differs
                           from attribute "name-corporate
                           (2)" in that corporate name
                           subject headings are not
                           included.)

   -->
  <xsl:template name="Author-name-corporate">
    <xsl:for-each select="marc:datafield[@tag='110']">
      <z:index name="Author-name-corporate:w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

   <!--
Author-name-         1006  A meeting of individuals or     111, 411, 711, 811
conference                 representatives of various
                           bodies for the purpose of
                           discussing topics of common
                           interest. (Differs from
                           attribute "name-conference
                           (3)" in that conference name
                           subject headings are not
                           included.)

   -->
  <xsl:template name="Author-name-conference">
    <xsl:for-each select="marc:datafield[@tag='111']">
      <z:index name="Author-name-conference:w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

   <!--
Author-name-personal 1004  A person's real name,           100, 400, 700, 800
                           pseudonym, title of nobility
                           nickname, or initials.
                           (Differs from attribute
                           "name-personal (1)" in that
                           personal name subject headings
                           are not included.)

   -->

  <xsl:template name="Author-name-personal">
    <xsl:for-each select="marc:datafield[@tag='100']">
      <xsl:for-each select="marc:subfield[@code='a']">
        <z:index name="Author-name-personal:w">
          <xsl:value-of select="."/>
        </z:index>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:for-each select="marc:datafield[@tag='100']">
      <z:index name="Author-name-personal:p">
        <xsl:value-of select="marc:subfield[@code='a']"/>
        <xsl:text> </xsl:text>
        <xsl:value-of select="marc:subfield[@code='d']"/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

   <!--
Author-Title-Subject 1036  An author or a title or a        1XX, 2XX, 4XX,
                           subject.                         6XX, 7XX, 8XX

            Note: When the Use attribute is Author-name-and-title (1000)
            the term contains both an author name and a title.  When the
            Use attribute is Author-Title-Subject (1036), the term
            contains an author name or a title or a subject.

   -->
   <!--
Body of text         1010  Used in full-text searching to
                           indicate that the term is to
                           be searched only in that
                           portion of the record that the
                           target considers the body of
                           the text, as opposed to some
                           other discriminated part such
                           as a headline, title, or
                           abstract.

   -->
   <!--
Classification-Bliss   15  A classification number from
                           the Bliss Classification,
                           developed by Henry Evelyn
                           Bliss.

   -->
   <!--
Classification-Dewey   13  A classification number from    082
                           the Dewey Decimal
                           Classification, developed by
                           Melvyl Dewey.

   -->
   <!--
Classification-        50  A classification number         086
government-publication     assigned to a government
                           document by a government
                           agency at any level (e.g.,
                           state, national,
                           international).

   -->
   <!--
Classification-LC      16  A classification number from    050
                           the US Library of Congress
                           Classification.

   -->
   <!--
Classification-local   20  A local classification
                           number from a system not
                           specified elsewhere in this
                           list of attributes.

   -->
   <!--
Classification-NAL     18  A classification number from    070
                           the US National Agriculture
                           Library Classification.

   -->
   <!--
Classification-NLM     17  A classification number from    060
                           the US National Library of
                           Medicine Classification.

   -->
   <!--
Classification-MOS     19  A classification number from
                           Mathematics Subject
                           Classification, compiled
                           in the Editorial Offices of
                           Mathematical Reviews and
                           Zentralblatt fur Mathematik.

   -->
   <!--
Classification-UDC     14  A classification number from    080
                           Universal Decimal
                           Classification, a system based
                           on the Dewey Decimal
                           Classification.

   -->
   <!--
Code-bib-level       1021  A one-character alphabetic       Leader/07
                           code indicating the
                           bibliographic level such as
                           monograph, serial or collection
                           of the record.

   -->
   <!--
Code-geographic-area   55  A code that indicates the       043
                           geographic area(s) that appear
                           or are implied in the headings
                           assigned to the item during
                           cataloging.

   -->
   <!--
Code-geographic-     1022  A code that represents the      052
class                      geographic area and if
                           applicable the geographic
                           subarea covered by an item.
                           The codes are derived from
                           the LC Classification-Class G
                           and the expanded Cutter number
                           list.

   -->
   <!--
Code-institution       56  An authoritative-agency         040, 852$a
                           symbol for an institution
                           that is the source of the
                           record or the holding
                           location.  The code space is
                           defined by the target.

   -->
   <!--
Code-language          54  A code that indicates the       008/35-37, 041
                           language of the item.
                           The codes are defined by the
                           target.

   -->
   <!--
Code-map-scale       1024  Coded form of cartographic      034
                           mathematical data, including
                           scale, projection and/or
                           coordinates related to the
                           item.

   -->
   <!--
Code-microform-        61  The code specifying the         007/11
generation                 generation of a microform.

   -->
   <!--
Code-record-type     1001  A code that specifies the       Leader/06
                           characteristics and defines
                           the components of the record.
                           The codes are target-specific.

   -->
   <!--
Concept-reference    1015  Used within Z39.50-1988;
                           included here for historical
                           reasons but its use is
                           deprecated.

   -->
   <!--
Concept-text         1014  Used within Z39.50-1988;
                           included here for historical
                           reasons but its use is
                           deprecated.

   -->
   <!--
Content-type         1034  The type of materials           derived value
                           contained in the item or        from 008/24-27
                           publication.  For example:
                           review, catalog, encyclopedia,
                           directory.

   -->
   <!--
Control number-BNB     10  Character string that uniquely  015
                           identifies a record in the
                           British National Bibliography.

   -->
   <!--
Control number-BNF     11  Character string that uniquely  015
                           identifies a record in the
                           Bibliotheque Nationale Francais.

   -->
   <!--
Control number-DB      52  Character string that uniquely  015
                           identifies a record in the
                           Deutsche Bibliothek.

   -->
   <!--
Control number-LC       9  Character string that uniquely  010, 011
                           identifies a record in the
                           Library of Congress database.

   -->
   <!--
Control number-local   12  Character string that uniquely  001, 035
                           identifies a record in a local
                           system (i.e., any system that
                           is not one of the four listed
                           above).

   -->
   <!--
Date                   30  The point of time at which      005, 008/00-05,
                           a transaction or event          008/07-10, 260$c,
                           takes place.                    008/11-14, 033,etc.

   -->
   <!--
Date-publication       31  The date (usually year) in      008/07-10, 260$c
                           which a document is published.  046, 533$d

   -->
   <!--
Date-acquisition       32  The date when a document was    541$d
                           acquired.

   -->
   <!--
Date/time added to   1011  The date and time that a        008/00-05
database                   record was added to the
                           database.

   -->
   <!--
Date/time last       1012  The date and time a record      005
modified                   was last updated.

   -->
   <!--
Identifier         1013  Used in full-text searching
authority/format           to indicate to the target
                           system the format of the
                           document that should be
                           returned to the originating
                           system.  The attribute carries
                           not only the format code, but
                           also the authority (e.g.,
                           system) that assigned that
                           code.

   -->
   <!--
Identifier-CODEN       60  A six-character, unique,        030
                           alphanumeric code assigned
                           to serial and monographic
                           publications by the CODEN
                           section of the Chemical
                           Abstracts Service.

   -->
   <!--
Identifier-document  1032  A persistent identifier, or
                           Doc-ID, assigned by a server,
                           that uniquely identifies a
                           document on that server.

   -->
   <!--
Identifier-ISBN         7  International Standard Book     020
                           Number - internationally
                           agreed upon number that
                           identifies a book uniquely.
                           Cf. ANSI/NISO Z39.21 and
                           ISO 2108.

   -->
  <xsl:template name="ISBN">
    <xsl:for-each select="marc:datafield[@tag='020']/marc:subfield[@code='a']">
      <z:index name="ISBN:n">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

   <!--
Identifier-ISSN         8  International Standard Serial   022, 4XX$x,
                           Number - internationally       7XX$x
                           agreed upon number that
                           identifies a serial uniquely.
                           Cf. ANSI/NISO z39.9 and
                           ISO 3297.

   -->
  <xsl:template name="ISSN">
    <xsl:for-each select="marc:datafield[@tag='022']">
      <z:index name="ISSN:n">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

   <!--
Identifier-legal-      49  The copyright registration      017
deposit                    number that is assigned to
                           an item when the item is
                           deposited for copyright.

   -->
   <!--
Identifier-local-call  53  Call number (e.g., shelf location)
                           assigned by a local system
                           (not a classification number).

   -->
   <!--
Identifier-national-   48  Character string that uniquely  015
bibliography               identifies a record in a
                           national bibliography.

   -->
   <!--
Identifier-publisher-  51  A formatted number assigned     028
for-music                  by a publisher to a sound
                           recording or to printed music.

   -->
   <!--
Identifier-report    1027  A report number assigned to     027, 088
                           the item. This number could be
                           the STRN (Standard Technical
                           Report Number) or another
                           report number.
                           Cf. ANSI/NISO Z39.23 and
                           ISO 10444.

   -->
   <!--
Identifier-standard  1007  Standard numbers such as ISBN,  010, 011, 015, 017,
                           ISSN, music publishers          018, 020, 022, 023,
                           numbers, CODEN, etc., that      024, 025, 027, 028,
                           are indexed together in many    030, 035, 037
                           online public-access catalogs.

   -->
   <!--
Identifier-stock     1028  A stock number that could be    037
                           used for ordering the item.

   -->
   <!--
Identifier-thematic  1030  The numeric designation for a   $n in the following:
                           part/section of a work such as  130, 240, 243, 630,
                           the serial, opus or thematic    700, 730
                           index number.

   -->
   <!--
Indexed-by           1023  For serials, a publication      510
                           in which the serial has been
                           indexed and/or abstracted.

   -->
   <!--
Material-type        1031  A free-form string, more        derived value from
                           specific than the one-letter    Leader/06-07, 007,
                           code in Leader/06, that         008, and 502
                           describes the material type
                           of the item, e.g., cassette,
                           kit, computer database,
                           computer file.

   -->
   <!--
Music-key            1025  A statement of the key in       $r in the following:
                           which the music is written.     130, 240, 243, 630,
                                                           700, 730

   -->
   <!--
Name                 1002  The name of a person, corporate 100, 110, 111, 400,
                           body, conference, or meeting.   410, 411, 600, 610,
                           (Subject name headings are      611, 700, 710, 711,
                           included.)                      800, 810, 811

   -->
   <!--
Name-and-title         57  The name of a person, corporate 100/2XX, 110/2XX,
                           body, conference, or meeting,   111/2XX, subfields
                           and the title of an item.       $a & $t in
                           (Subject name headings are      following: 400,410,
                           included.)  The syntax of the   411, 600, 610, 611,
                           name-title combination is up    700, 710, 711, 800,
                           to the target, unless used      810, 811
                           with the Structure attribute
                           Key (see below).

   -->
   <!--
Name-corporate          2  An organization or a group      110, 410, 610, 710,
                           of persons that is identified   810
                           by a particular name. (Subject
                           name headings are included.)

   -->
   <!--
Name-conference         3  A meeting of individuals or     111, 411, 611, 711
                           representatives of various      811
                           bodies for the purpose of
                           discussing topics of common
                           interest.  (Subject name
                           headings are included.)

   -->
   <!--
Name-editor          1020  A person who prepared for       100 $a or 700 $a when
                           publication an item that is     the corresponding $e
                           not his or her own.             contains value 'ed.'

   -->
   <!--
Name-geographic        58  Name of a country,              651
                           jurisdiction, region, or
                           geographic feature.

   -->
   <!--
Name-geographic-place- 59  City or town where an item      008/15-17, 260$a
publication                was published.

   -->
   <!--
Name-personal           1  A person's real name,           100, 400, 600, 700,
                           pseudonym, title of nobility    800
                           nickname, or initials.

   -->
   <!--
Name-publisher       1018  The organization responsible    260$b
                           for the publication of the
                           item.

   -->
   <!--
Note                   63  A concise statement in which    5XX
                           such information as extended
                           physical description,
                           relationship to other works,
                           or contents may be recorded.

   -->
   <!--
Record-source        1019  The USMARC code or name of the  008/39, 040
                           organization(s) that created
                           the original record, assigned
                           the USMARC content designation
                           and transcribed the record into
                           machine-readable form, or
                           modified the existing USMARC
                           record; the cataloging source.

   -->
   <!--
Server-choice        1017  The target substitutes one or
                           more access points.  The origin
                           leaves the choice to the target.

       Notes on relationship of Any and Server-choice:
        (1) When the origin uses 'Server-choice' it is asking the target
            to select one or more access points, and to use its best
            judgment in making that selection.  When 'Any' is used,
            there is no selection process involved; the target is to
            apply all of the (appropriate) supported Use attributes.
            The origin is asking the target to make a choice of access
            points.
        (2) The target might support 'Any' and not 'Server-choice', or
            vice versa, or both.  If the target supports both, when the
            origin uses 'Server-choice', the target might choose 'Any';
            however, it might choose any other Use attribute.

   -->
   <!--
Subject                21  The primary topic on which a    600, 610, 611, 630,
                           work is focused.                650, 651, 653, 654,
                                                           655, 656, 657, 69X

   -->
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
      <z:index name="Subject-heading:w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
    <xsl:for-each select="marc:datafield[@tag='600']
                          |marc:datafield[@tag='650']
                          |marc:datafield[@tag='651']
                          |marc:datafield[@tag='653']">
      <z:index name="Subject-heading:w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>

   <!--
Subject-BDI            23  Subject headings from
                           Bibliotek Dokumentasjon
                           Informasjon - a controlled
                           subject vocabulary used and
                           maintained by the five Nordic
                           countries (Denmark, Finland,
                           Iceland, Norway, and Sweden).

   -->
   <!--
Subject-INSPEC         24  Subject headings from           600i2, 610i2,
                           Information Services for the    611i2, 630i2,
                           Physics and Engineering         650i2, 651i2
                           Communities - the Information
                           Services Division of the
                           Institution of Electrical
                           Engineers.

   -->
   <!--
Subject-LC             27  Subject headings from           600i0, 610i0,
                           US Library of Congress          611i0, 630i0,
                           Subject Headings.               650i0, 651i0

   -->
   <!--
Subject-LC-          1008  Subject headings, for use       600i1, 610i1,
children's                 with children's literature,     611i1, 630i1,
                           that conform to the             650i1, 651i1
                           formulation guidelines in
                           the "AC Subject Headings"
                           section of the Library of
                           Congress Subject Headings.

   -->
   <!--
Subject-local          29  Subjects headings defined
                           locally.

   -->
   <!--
Subject-MESH           25  Subject headings from           600i2, 610i2,
                           Medical Subject Headings -     611i2, 630i2,
                           maintained by the US National   650i2, 651i2
                           Library of Medicine.

   -->
   <!--
Subject-name-        1009  A person's real name,           600
personal                   pseudonym, title of nobility
                           nickname, or initials that
                           appears in a subject heading.

   -->
   <!--
Subject-PA             26  Subject headings from           600i2, 610i2,
                           Thesaurus of Psychological      611i2, 630i2,
                           Index Terms - maintained       650i2, 651i2
                           by the Retrieval Services Unit
                           of the American Psychological
                           Association.

   -->
   <!--
Subject-PRECIS         45  Subject headings from
                           PREserved Context Index
                           System - a string of indexing
                           terms set down in a prescribed
                           order, each term being preceded
                           by a manipulation code which
                           governs the production of
                           pre-coordinated subject index
                           entries under selected terms -
                           maintained by the British
                           Library.

   -->
   <!--
Subject-RAMEAU         22  Subject headings from
                           Repertoire d'authorite de
                           matieres encyclopedique
                           unifie - maintained by the
                           Bibliotheque Nationale
                           (France).
   -->
   <!--
Subject-RSWK           46  Subject headings from
                           Regeln fur den
                           Schlagwortkatalog -
                           maintained by the Deutsches
                           Bibliotheksinstitut.

   -->
   <!--
Subject-RVM            28  Subject headings from           600i6, 610i6,
                           Repertoire des vedettes-        611i6, 630i6,
                           matiere - maintained by the    650i6, 651i6
                           Bibliotheque de l'Universite
                           de Laval.

   -->
   <!--
Subject-subdivision    47  An extension to a subject       6XX$x, 6XX$y,
                           heading indicating the form,    6XX$z
                           place, period of time treated,
                           or aspect of the subject
                           treated.

   -->
   <!--
Title                   4  A word, phrase, character,      130, 21X-24X, 440,
                           or group of characters,         490, 730, 740, 830,
                           normally appearing in an item,  840, subfield $t
                           that names the item or the      in the following:
                           work contained in it.           400, 410, 410, 600,
                                                           610, 611, 700, 710,
                                                           711, 800, 810, 811

   -->
  <xsl:template name="Title">
    <xsl:for-each select="marc:datafield[@tag='245']/marc:subfield[@code='a']">
      <z:index name="Title:w">
        <xsl:value-of select="."/>
      </z:index>
    </xsl:for-each>
  </xsl:template>


   <!--
Title-abbreviated      43  Shortened form of the title;    210, 211 (obs.),
                           either assigned by national     246
                           centers under the auspices of
                           the International Serials Data
                           System, or a title (such as an
                           acronym) that is popularly
                           associated with the item.

   -->
   <!--
Title-added-title-page 37  A title on a title page         246i5
                           preceding or following the
                           title page chosen as the basis
                           for the description of the
                           item.  It may be more general
                           (e.g., a series title page),
                           or equally general (e.g., a
                           title page in another
                           language).

   -->
   <!--
Title-caption          38  A title given at the beginning  246i6
                           of the first page of the text.

   -->
   <!--
Title-collective       34  A title proper that is an       243
                           inclusive title for an item
                           containing several works.

   -->
   <!--
Title-cover            36  The title printed on the        246i4
                           cover of an item as issued.

   -->
   <!--
Title-expanded         44  An expanded (or augmented)      214 (obs.), 246
                           title has been enlarged with
                           descriptive words by the
                           cataloger to provide
                           additional indexing and
                           searching capabilities.

   -->
   <!--
Title-former           42  A former title or title         247, 780
                           variation when one
                           bibliographic record
                           represents all issues of
                           a serial that has changed
                           title.

   -->
   <!--
Title-host-item      1033  The title of the item            773$t
                           containing the part
                           described in the record, for
                           example, a journal title
                           when the record describes an
                           article in the journal.

   -->
   <!--
Title-key              33  The unique name assigned to     222
                           a serial by the International
                           Serials Data System (ISDS).

   -->
   <!--
Title-other-variant    41  A variation from the title      212 (obs.), 246i3,
                           page title appearing elsewhere  247, 740
                           in the item (e.g., a variant
                           cover title, caption title,
                           running title, or title from
                           another volume) or in another
                           issue.

   -->
   <!--
Title-parallel         35  The title proper in another     246i1
                           language and/or script.

   -->
   <!--
Title-related-       1026  Serial titles related to this   247, 780, 785
periodical                 item, either the immediate
                           predecessor or the immediate
                           successor.

   -->
   <!--
Title-running          39  A title, or abbreviated title,  246i7
                           that is repeated at the head
                           or foot of each page or leaf.

   -->
   <!--
Title-series            5  Collective title applying to    440, 490, 830, 840,
                           a group of separate, but        subfield $t in the
                           related, items.                 following: 400,410,
                                                           411, 800, 810, 811

   -->
   <!--
Title-spine            40  A title appearing on the        246i8
                           spine of an item.

   -->
   <!--
Title-uniform           6  The particular title by which   130, 240, 730,
                           a work is to be identified      subfield $t in the
                           for cataloging purposes.        following: 700,710,
                                                           711
-->




</xsl:stylesheet>
