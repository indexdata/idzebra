## $Id: Makefile.am,v 1.8 2002-04-09 19:41:50 adam Exp $
docdir=$(pkgdatadir)/doc

doc_DATA = zebra.html zebra.pdf

SUPPORTFILES = \
 zebrahtml.dsl.in \
 zebraphp.dsl.in \
 zebraprint.dsl.in \
 xml.dcl
XMLFILES = \
 zebra.xml.in \
 introduction.xml \
 installation.xml \
 quickstart.xml \
 administration.xml \
 zebraidx.xml \
 server.xml \
 recordmodel.xml \
 license.xml \
 indexdata.xml
 
EXTRA_DIST = $(SUPPORTFILES) $(XMLFILES) zebra.pdf zebra.html

install-data-hook:
	for f in *.html; do \
		$(INSTALL_DATA) $$f $(DESTDIR)$(docdir)/$$f; \
	done	

zebra.html: $(XMLFILES)
	jade -E14 -d zebrahtml.dsl -t sgml xml.dcl zebra.xml

zebra.php: $(XMLFILES)
	jade -E14 -d zebraphp.dsl -t sgml xml.dcl zebra.xml

zebra.pdf: $(XMLFILES)
	jade -E14 -d zebraprint.dsl -t tex xml.dcl zebra.xml
	pdfjadetex zebra.tex
	pdfjadetex zebra.tex
	pdfjadetex zebra.tex

gils.txt: gils.sgml
	sgml2txt -f gils.sgml

gils.ps: gils.sgml
	sgml2latex -o ps -p a4 gils.sgml

gils.html: gils.sgml
	sgml2html gils.sgml

dist-hook:
	cp $(srcdir)/*.html $(distdir)

clean-data-hook:
	rm -f [0-9]* *.bak
	
