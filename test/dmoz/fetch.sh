#!/bin/sh
if test ! -f content.rdf.u8; then
   wget http://dmoz.org/rdf/content.rdf.u8.gz
   gunzip content.rdf.u8.gz
fi
