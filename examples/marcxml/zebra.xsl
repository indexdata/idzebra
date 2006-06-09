<?xml version="1.0" encoding="UTF-8"?>

<!-- 
$Id: zebra.xsl,v 1.1 2006-06-09 20:46:38 marc Exp $
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

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:z="http://indexdata.dk/zebra/xslt/1" 
                version="1.0">

  <xsl:param name="id" select="''"/>
  <xsl:param name="filename" select="''"/>
  <xsl:param name="rank" select="''"/>
  <xsl:param name="score" select="''"/>
  <xsl:param name="schema" select="''"/>
  <xsl:param name="size" select="''"/>

  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>

  <!-- match on any record -->
  <xsl:template match="/">
    <z:info z:id="{$id}"
            z:filename="{$filename}"
            z:rank="{$rank}"
            z:score="{$score}"
            z:schema="{$schema}"
            z:size="{$size}"/>
  </xsl:template>

</xsl:stylesheet>
