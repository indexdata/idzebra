## $Id: Makefile.am,v 1.17 2002-10-30 12:55:18 adam Exp $
docdir=$(datadir)/doc/@PACKAGE@

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

HTMLFILES = administration.html apps.html configuration-file.html \
 data-model.html example1.html example2.html examples.html features.html \
 file-ids.html formats.html future.html generic-ids.html indexdata.html \
 installation.html internal-representation.html introduction.html \
 license.html locating-records.html protocol-support.html quick-start.html \
 record-model.html register-location.html server.html shadow-registers.html \
 simple-indexing.html support.html zebra.html zebraidx.html

doc_DATA = $(HTMLFILES) zebra.pdf
man_MANS = zebraidx.1 zebrasrv.8

EXTRA_DIST = $(SUPPORTFILES) $(XMLFILES) zebra.pdf \
 $(man_MANS) zebraidx-man.sgml zebrasrv-man.sgml

zebraidx.1: zebraidx-man.sgml zebraidx-options.xml
	if test ! -f zebraidx-options.xml ; then ln -s $(srcdir)/zebraidx-options.xml .; fi
	docbook2man $(srcdir)/zebraidx-man.sgml

zebrasrv.8: zebrasrv-man.sgml
	docbook2man $(srcdir)/zebrasrv-man.sgml

$(HTMLFILES): $(XMLFILES)
	jade -E14 -D $(srcdir) -d zebrahtml.dsl -t sgml $(srcdir)/xml.dcl zebra.xml

zebra.php: $(XMLFILES)
	jade -E14 -D $(srcdir) -d zebraphp.dsl -t sgml $(srcdir)/xml.dcl zebra.xml

zebra.pdf: $(XMLFILES)
	jade -E14 -D $(srcdir) -d zebraprint.dsl -t tex $(srcdir)/xml.dcl zebra.xml
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
	cp *.html $(distdir)

clean-data-hook:
	rm -f [0-9]* *.bak

