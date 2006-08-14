## $Id: Makefile.am,v 1.23.2.2 2006-08-14 13:37:18 adam Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUPPORTFILES = \
 zebrahtml.dsl \
 zebraphp.dsl \
 zebraprint.dsl \
 tkl.xsl \
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
 zebraidx-options.xml \
 zebraidx-commands.xml

HTMLFILES =  \
 administration.html apps.html configuration-file.html data-model.html \
 example1.html example2.html examples.html features.html file-ids.html \
 formats.html future.html generic-ids.html indexdata.html installation.html \
 installation.win32.html internal-representation.html introduction.html \
 license.html locating-records.html protocol-support.html quick-start.html \
 record-model.html register-location.html server.html shadow-registers.html \
 simple-indexing.html support.html zebra.html zebraidx.html 

PNGFILES=zebra.png
EPSFILES=zebra.eps

MANFILES=zebraidx$(PACKAGE_SUFFIX).1 zebrasrv$(PACKAGE_SUFFIX).8

REFFILES=zebraidx-man.sgml zebrasrv-man.sgml

doc_DATA = $(HTMLFILES) zebra.pdf $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(SUPPORTFILES) $(REFFILES) \
	$(doc_DATA) $(EPSFILES) $(man_MANS) $(REFFILES) \
	zvrank.txt marc_indexing.xml

zebraidx$(PACKAGE_SUFFIX).1: zebraidx-man.sgml zebraidx-options.xml zebraidx-commands.xml
	if test ! -f zebraidx-options.xml ; then ln -s $(srcdir)/zebraidx-options.xml .; fi
	if test ! -f zebraidx-commands.xml ; then ln -s $(srcdir)/zebraidx-commands.xml .; fi
	docbook2man $(srcdir)/zebraidx-man.sgml
	mv zebraidx.1 zebraidx$(PACKAGE_SUFFIX).1

zebrasrv$(PACKAGE_SUFFIX).8: zebrasrv-man.sgml
	docbook2man $(srcdir)/zebrasrv-man.sgml
	mv zebrasrv.8 zebrasrv$(PACKAGE_SUFFIX).8

$(HTMLFILES): $(XMLFILES)
	jade -E14 -D $(srcdir) -d zebrahtml.dsl -t sgml $(srcdir)/xml.dcl zebra.xml

zebra.php: $(XMLFILES)
	jade -E14 -D $(srcdir) -d zebraphp.dsl -t sgml $(srcdir)/xml.dcl zebra.xml

zebra.pdf: $(XMLFILES)
	for i in $(PNGFILES); do \
		if test ! -f $$i; then ln -s $(srcdir)/$$i .; fi; \
	done
	jade -E14 -D $(srcdir) -d zebraprint.dsl -t tex $(srcdir)/xml.dcl zebra.xml
	pdfjadetex zebra.tex
	pdfjadetex zebra.tex
	pdfjadetex zebra.tex

index.tkl: $(XMLFILES) tkl.xsl
	xsltproc tkl.xsl zebra.xml

gils.txt: gils.sgml
	sgml2txt -f gils.sgml

gils.ps: gils.sgml
	sgml2latex -o ps -p a4 gils.sgml

gils.html: gils.sgml
	sgml2html gils.sgml

clean-data-hook:
	rm -f [0-9]* *.bak

dist-hook:
	for f in $(srcdir)/*.html; do \
		found=0; \
		b=`basename $$f`; \
		for h in $(HTMLFILES); do \
			if test "$$h" = "$$b"; then \
				found=1; \
			fi \
		done; \
		if test "$$found" = "0"; then \
			echo "$$h not found in HTMLFILES"; \
			exit 1; \
		fi \
	done

install-data-hook:
	cd $(DESTDIR)$(man1dir) && ln -sf zebraidx$(PACKAGE_SUFFIX).1 zebraidx.1
	cd $(DESTDIR)$(man8dir) && ln -sf zebrasrv$(PACKAGE_SUFFIX).8 zebrasrv.8
