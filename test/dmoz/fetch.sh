#!/bin/sh
# $Id: fetch.sh,v 1.2 2002-06-19 08:32:34 adam Exp $
if test ! -f content.rdf.u8; then
   wget http://dmoz.org/rdf/content.rdf.u8.gz
   gunzip content.rdf.u8.gz
fi
