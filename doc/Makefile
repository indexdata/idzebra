# $Id: Makefile,v 1.1 1995-11-27 14:23:52 adam Exp $
all: zebra.ps zebra.txt zebra.html

zebra.txt: zebra.sgml
	sgml2txt zebra.sgml

zebra.dvi: zebra.sgml
	sgml2ps -d zebra.sgml

zebra.ps: zebra.sgml
	sgml2ps zebra.sgml

zebra.html: zebra.sgml
	sgml2html zebra.sgml

clean:
	rm -f [0-9]* *.bak
	
