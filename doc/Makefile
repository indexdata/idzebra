# $Id: Makefile,v 1.4 1998-01-29 13:35:11 adam Exp $
all: zebra.tex zebra.txt zebra.html

zebra.txt: zebra.sgml
	sgml2txt zebra.sgml

zebra.tex: zebra.sgml
	sgml2latex zebra.sgml

zebra.html: zebra.sgml
	sgml2html zebra.sgml


gils.txt: gils.sgml
	sgml2txt gils.sgml

gils.tex: gils.sgml
	sgml2latex gils.sgml

gils.html: gils.sgml
	sgml2html gils.sgml

clean:
	rm -f [0-9]* *.bak
	
