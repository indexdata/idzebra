# $Id: Makefile.am,v 1.2 2000-04-06 09:03:28 adam Exp $
docdir=$(pkgdatadir)/doc

doc_DATA = zebra.sgml gils.sgml zebra.txt zebra.ps zebra.html \
 zebra-1.html zebra-2.html zebra-3.html zebra-4.html zebra-5.html \
 zebra-6.html zebra-7.html zebra-8.html zebra-9.html

EXTRA_DIST = zebra.sgml gils.sgml zebra.txt zebra.ps zebra.html \
 zebra-1.html zebra-2.html zebra-3.html zebra-4.html zebra-5.html \
 zebra-6.html zebra-7.html zebra-8.html zebra-9.html

zebra.txt: zebra.sgml
	sgml2txt -f zebra.sgml

zebra.ps: zebra.sgml
	sgml2latex -o ps -p a4 zebra.sgml

zebra.html: zebra.sgml
	sgml2html zebra.sgml


gils.txt: gils.sgml
	sgml2txt -f gils.sgml

gils.ps: gils.sgml
	sgml2latex -o ps -p a4 gils.sgml

gils.html: gils.sgml
	sgml2html gils.sgml

clean-hook:
	rm -f [0-9]* *.bak
	
