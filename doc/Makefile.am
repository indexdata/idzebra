## $Id: Makefile.am,v 1.10 2002-06-02 19:30:07 adam Exp $
docdir=$(datadir)/doc/@PACKAGE@

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
 
EXTRA_DIST = $(SUPPORTFILES) $(XMLFILES) zebra.pdf

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

dist-hook: zebra.html
	cp -f $(srcdir)/*.html $(distdir)

clean-data-hook:
	rm -f [0-9]* *.bak
	
