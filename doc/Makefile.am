# $Id: Makefile.am,v 1.1 2000-04-05 09:49:35 adam Exp $
docdir=$(pkgdatadir)/doc

doc_data = zebra.sgml gils.sgml

EXTRA_DIST = zebra.sgml gils.sgml

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
	
