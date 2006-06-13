## $Id: Makefile.am,v 1.48 2006-06-13 09:26:59 marc Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUBDIRS = common

XMLFILES =  \
   administration.xml \
   architecture.xml \
   examples.xml \
   idzebra-config-man.xml \
   indexdata.xml \
   installation.xml \
   introduction.xml \
   license.xml \
   marc_indexing.xml \
   querymodel.xml \
   quickstart.xml \
   recordmodel-alvisxslt.xml \
   recordmodel-grs.xml \
   server.xml \
   zebra.xml \
   zebraidx-commands.xml \
   zebraidx-man.xml \
   zebraidx-options.xml \
   zebraidx.xml \
   zebrasrv-man.xml \
   zebrasrv-options.xml \
   zebrasrv-synopsis.xml \
   zebrasrv-virtual.xml

HTMLFILES =  \
   administration-extended-services.html \
   administration-ranking.html \
   administration.html \
   apps.html \
   architecture-maincomponents.html \
   architecture-workflow.html \
   architecture.html \
   configuration-file.html \
   example1.html \
   example2.html \
   examples.html \
   features.html \
   file-ids.html \
   future.html \
   generic-ids.html \
   gfs-config.html \
   grs-exchange-formats.html \
   grs-internal-representation.html \
   htmlhelp.hhp \
   index.html \
   indexdata.html \
   installation.debian.html \
   installation.html \
   installation.win32.html \
   introduction.html \
   license.html \
   locating-records.html \
   protocol-support.html \
   querymodel-cql-to-pqf.html \
   querymodel-pqf.html \
   querymodel.html \
   quick-start.html \
   record-model-alvisxslt-conf.html \
   record-model-alvisxslt.html \
   record-model-grs-conf.html \
   record-model-grs.html \
   register-location.html \
   server-sru-support.html \
   server-sru.html \
   server.html \
   shadow-registers.html \
   simple-indexing.html \
   support.html \
   toc.hhc \
   zebraidx.html


PNGFILES=zebra.png
EPSFILES=zebra.eps

MANFILES=zebraidx.1 zebrasrv.8 idzebra-config.1

REFFILES=zebraidx-man.xml zebrasrv-man.xml idzebra-config-man.xml

doc_DATA = $(HTMLFILES) $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(REFFILES) \
	$(doc_DATA) $(EPSFILES) $(man_MANS) $(REFFILES) \
	marc_indexing.xml entities.ent local.ent.in

zebraidx.1: zebraidx-man.xml zebraidx-options.xml zebraidx-commands.xml
	$(MAN_COMPILE) $(srcdir)/zebraidx-man.xml

zebrasrv.8: zebrasrv-man.xml  zebrasrv-options.xml \
            zebrasrv-synopsis.xml zebrasrv-virtual.xml
	$(MAN_COMPILE) $(srcdir)/zebrasrv-man.xml

idzebra-config.1: idzebra-config-man.xml
	$(MAN_COMPILE) $(srcdir)/idzebra-config-man.xml

$(HTMLFILES): $(XMLFILES)
	$(HTML_COMPILE) $(srcdir)/zebra.xml

index.tkl: $(XMLFILES) common/tkl.xsl
	$(TKL_COMPILE) $(srcdir)/zebra.xml

zebra.pdf: $(XMLFILES)
	for i in $(PNGFILES); do \
		if test ! -f $$i; then cp $(srcdir)/$$i .; fi; \
	done
	jade -E14 -D $(srcdir) -d common/print.dsl -t tex $(srcdir)/common/xml.dcl zebra.xml
	pdfjadetex zebra.tex >/dev/null
	pdfjadetex zebra.tex >/dev/null
	pdfjadetex zebra.tex >/dev/null

gils.txt: gils.sgml
	sgml2txt -f gils.sgml

gils.ps: gils.sgml
	sgml2latex -o ps -p a4 gils.sgml

gils.html: gils.sgml
	sgml2html gils.sgml

clean-data-hook:
	rm -f [0-9]* *.bak

dist-hook:
	for f in *.html; do \
		found=0; \
		b=`basename $$f`; \
		for h in $(HTMLFILES); do \
			if test "$$h" = "$$b"; then \
				found=1; \
			fi \
		done; \
		if test "$$found" = "0"; then \
			echo "$$f not found in HTMLFILES"; \
			exit 1; \
		fi \
	done
