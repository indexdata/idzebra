# $Id: Makefile,v 1.5 1999-12-09 01:03:48 adam Exp $
all: zebra.tex zebra.txt zebra.html

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

clean:
	rm -f [0-9]* *.bak
	
