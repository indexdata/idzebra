# $Id: Makefile,v 1.2 1996-05-07 12:29:35 quinn Exp $
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

clean:
	rm -f [0-9]* *.bak
	
