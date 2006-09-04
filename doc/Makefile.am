## $Id: Makefile.am,v 1.59 2006-09-04 12:11:40 adam Exp $
docdir=$(datadir)/doc/$(PACKAGE)$(PACKAGE_SUFFIX)

SUBDIRS = common

XMLFILES =  \
   administration.xml \
   architecture.xml \
   examples.xml \
   field-structure.xml \
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

HTMLFILES =  index.html

PNGFILES=zebra.png
EPSFILES=zebra.eps

MANFILES=zebraidx$(PACKAGE_SUFFIX).1 \
	zebrasrv$(PACKAGE_SUFFIX).8 \
	idzebra-config$(PACKAGE_SUFFIX).1

REFFILES=zebraidx-man.xml zebrasrv-man.xml idzebra-config-man.xml

doc_DATA = $(HTMLFILES) $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(REFFILES) \
	$(doc_DATA) $(EPSFILES) $(man_MANS) $(REFFILES) \
	marc_indexing.xml entities.ent local.ent.in

zebraidx$(PACKAGE_SUFFIX).1: zebraidx-man.xml zebraidx-options.xml zebraidx-commands.xml
	$(MAN_COMPILE) $(srcdir)/zebraidx-man.xml
	mv zebraidx.1 zebraidx$(PACKAGE_SUFFIX).1

zebrasrv$(PACKAGE_SUFFIX).8: zebrasrv-man.xml  zebrasrv-options.xml \
            zebrasrv-synopsis.xml zebrasrv-virtual.xml
	$(MAN_COMPILE) $(srcdir)/zebrasrv-man.xml
	mv zebrasrv.8 zebrasrv$(PACKAGE_SUFFIX).8

idzebra-config$(PACKAGE_SUFFIX).1: idzebra-config-man.xml
	$(MAN_COMPILE) $(srcdir)/idzebra-config-man.xml
	mv idzebra-config.1 idzebra-config$(PACKAGE_SUFFIX).1

$(HTMLFILES): $(XMLFILES)
	rm -f *.html
	$(HTML_COMPILE) $(srcdir)/zebra.xml

index.tkl: $(XMLFILES)
	rm -f *.tkl
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
	if test -d index.html; then d=.; else d="$(srcdir)"; fi; \
	for p in $$d/*.html; do \
		cp $$p $(distdir); \
	done

install-data-hook:
	cd $(DESTDIR)$(man1dir) && ln -sf zebraidx$(PACKAGE_SUFFIX).1 zebraidx.1
	cd $(DESTDIR)$(man8dir) && ln -sf zebrasrv$(PACKAGE_SUFFIX).8 zebrasrv.8
	cd $(DESTDIR)$(man1dir) && ln -sf idzebra-config$(PACKAGE_SUFFIX).1 idzebra-config.1
	if test -d index.html; then d=.; else d="$(srcdir)"; fi; \
	for p in $$d/*.html; do \
		$(docDATA_INSTALL) $$p $(DESTDIR)/$(docdir); \
	done
