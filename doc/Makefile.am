## $Id: Makefile.am,v 1.4 2002-04-08 13:50:13 adam Exp $
docdir=$(pkgdatadir)/doc

doc_data = zebra.pdf zebra.html

SUPPORTFILES = zebrahtml.dsl.in zebraprint.dsl.in xml.dsl
 
EXTRA_DIST = zebra.sgml gils.sgml zebra.txt zebra.ps zebra.html \
 zebra-1.html zebra-2.html zebra-3.html zebra-4.html zebra-5.html \
 zebra-6.html zebra-7.html zebra-8.html zebra-9.html \
 zebra.xml.in $(SUPPORTFILES)

zebra.pdf: zebra.xml
	jade -E14 -d zebraprint.dsl -t tex xml.dcl zebra.xml
	pdfjadetex zebra.tex
	pdfjadetex zebra.tex
	pdfjadetex zebra.tex

zebra.html: zebra.xml
	jade -E14 -d zebrahtml.dsl -t sgml xml.dcl zebra.xml

gils.txt: gils.sgml
	sgml2txt -f gils.sgml

gils.ps: gils.sgml
	sgml2latex -o ps -p a4 gils.sgml

gils.html: gils.sgml
	sgml2html gils.sgml

clean-hook:
	rm -f [0-9]* *.bak
	
