# $Id: Makefile,v 1.3 1996-05-13 13:25:51 quinn Exp $
all: zebra.ps zebra.txt zebra.html

zebra.txt: zebra.sgml
	sgml2txt zebra.sgml

zebra.dvi: zebra.sgml
	sgml2ps -d zebra.sgml

zebra.ps: zebra.sgml
	sgml2ps zebra.sgml

zebra.html: zebra.sgml
	sgml2html zebra.sgml


gils.txt: gils.sgml
	sgml2txt gils.sgml

gils.ps: gils.sgml
	sgml2ps gils.sgml

gils.html: gils.sgml
	sgml2html gils.sgml

clean:
	rm -f [0-9]* *.bak
	
