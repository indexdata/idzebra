## $Id: Makefile.am,v 1.73 2007-07-10 09:41:12 adam Exp $
SUBDIRS = common

XMLMAN = zebraidx.xml zebrasrv.xml idzebra-config.xml

XMLFILES =  \
   administration.xml \
   architecture.xml \
   examples.xml \
   field-structure.xml \
   indexdata.xml \
   installation.xml \
   introduction.xml \
   license.xml \
   marc_indexing.xml \
   querymodel.xml \
   quickstart.xml \
   recordmodel-domxml.xml \
   recordmodel-alvisxslt.xml \
   recordmodel-grs.xml \
   manref.xml \
   zebra.xml \
   zebrasrv-options.xml \
   zebrasrv-synopsis.xml \
   zebrasrv-virtual.xml 

HTMLFILES = index.html

PNGFILES = zebra.png domfilter.png
EPSFILES = zebra.eps domfilter.eps
PDFFILES = zebra.pdf domfilter.pdf


MANFILES=zebraidx$(PACKAGE_SUFFIX).1 \
	zebrasrv$(PACKAGE_SUFFIX).8 \
	idzebra-config$(PACKAGE_SUFFIX).1

doc_DATA = $(HTMLFILES) $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(XMLMAN) \
	$(doc_DATA) zebra.eps $(man_MANS) \
	marc_indexing.xml entities.ent local.ent.in domfilter.svg

zebraidx$(PACKAGE_SUFFIX).1: zebraidx.xml
	$(MAN_COMPILE) $(srcdir)/zebraidx.xml
	mv zebraidx.1 zebraidx$(PACKAGE_SUFFIX).1

zebrasrv$(PACKAGE_SUFFIX).8: zebrasrv.xml  zebrasrv-options.xml \
            zebrasrv-synopsis.xml zebrasrv-virtual.xml
	$(MAN_COMPILE) $(srcdir)/zebrasrv.xml
	mv zebrasrv.8 zebrasrv$(PACKAGE_SUFFIX).8

idzebra-config$(PACKAGE_SUFFIX).1: idzebra-config.xml
	$(MAN_COMPILE) $(srcdir)/idzebra-config.xml
	mv idzebra-config.1 idzebra-config$(PACKAGE_SUFFIX).1

$(HTMLFILES): $(XMLFILES) $(PNGFILES)
	rm -f *.html
	$(HTML_COMPILE) $(srcdir)/zebra.xml

index.tkl: $(XMLFILES)
	rm -f *.tkl
	$(TKL_COMPILE) $(srcdir)/zebra.xml

zebra.pdf: $(XMLFILES) domfilter.pdf
	for i in $(PNGFILES); do \
		if test ! -f $$i; then cp $(srcdir)/$$i .; fi; \
	done
	dblatex $(srcdir)/zebra.xml

manref.xml: $(XMLMAN) $(srcdir)/common/ref2dbinc.xsl
	rm -f manref.xml
	for i in $(XMLMAN); do \
		xsltproc $(srcdir)/common/stripref.xsl $(srcdir)/$$i | sed 1d >> manref.xml; \
	done

gils.txt: gils.sgml
	sgml2txt -f gils.sgml

gils.ps: gils.sgml
	sgml2latex -o ps -p a4 gils.sgml

gils.html: gils.sgml
	sgml2html gils.sgml

clean-data-hook:
	rm -f [0-9]* *.bak

dist-hook:
	if test -f index.html; then d=.; else d="$(srcdir)"; fi; \
	for p in $$d/*.html; do \
		cp $$p $(distdir); \
	done

install-data-hook:
	cd $(DESTDIR)$(man1dir) && ln -sf zebraidx$(PACKAGE_SUFFIX).1 zebraidx.1
	cd $(DESTDIR)$(man8dir) && ln -sf zebrasrv$(PACKAGE_SUFFIX).8 zebrasrv.8
	cd $(DESTDIR)$(man1dir) && ln -sf idzebra-config$(PACKAGE_SUFFIX).1 idzebra-config.1
	if test -f index.html; then d=.; else d="$(srcdir)"; fi; \
	for p in $$d/*.html; do \
		$(docDATA_INSTALL) $$p $(DESTDIR)$(docdir); \
	done

uninstall-hook:
	rm -r $(DESTDIR)/$(docdir)

domfilter.png: domfilter.svg
	inkscape --export-png=$@ --export-width=600 $?

domfilter.eps: domfilter.svg
	inkscape --export-eps=$@ --export-bbox-page $?

.eps.pdf:
	epstopdf -hires $?

