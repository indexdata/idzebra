## $Id: Makefile.am,v 1.14 2002-09-25 09:50:20 adam Exp $
docdir=$(datadir)/doc/@PACKAGE@

doc_DATA = zebra.html zebra.pdf
man_MANS = zebraidx.1 zebrasrv.8

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
 examples.xml \
 administration.xml \
 zebraidx.xml \
 server.xml \
 recordmodel.xml \
 license.xml \
 indexdata.xml \
 zebraidx-options.xml

EXTRA_DIST = $(SUPPORTFILES) $(XMLFILES) zebra.pdf \
 $(man_MANS) zebraidx-man.sgml zebrasrv-man.sgml

install-data-hook:
	for f in $(srcdir)/*.html; do \
		$(INSTALL_DATA) $$f $(DESTDIR)$(docdir)/$$f; \
	done

zebraidx.1: zebraidx-man.sgml zebraidx-options.xml
	cd $(srcdir); docbook2man zebraidx-man.sgml

zebrasrv.8: zebrasrv-man.sgml
	cd $(srcdir); docbook2man zebrasrv-man.sgml

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

