## $Id: Makefile.am,v 1.17 2004-09-15 09:17:25 adam Exp $

AUTOMAKE_OPTIONS=foreign

SUBDIRS=util bfile dfa dict isams isamb isamc rset data1 \
 recctrl tab index test examples include doc

aclocaldir=$(datadir)/aclocal
aclocal_DATA = idzebra.m4

SPEC_FILE=$(PACKAGE).spec

EXTRA_DIST= README NEWS $(SPEC_FILE) idzebra.spec.in \
 idzebra-config.in idzebra.m4

dist-hook:
	cp $(srcdir)/LICENSE* $(distdir)
	mkdir $(distdir)/win
	for i in $(srcdir)/win/*; do \
		if test -f $$i; then \
			cp $$i $(distdir)/win; \
		fi; \
	done
	mkdir $(distdir)/debian
	cp $(srcdir)/debian/control $(distdir)/debian
	cp $(srcdir)/debian/changelog $(distdir)/debian
	cp $(srcdir)/debian/rules $(distdir)/debian
	cp $(srcdir)/debian/postinst $(distdir)/debian
	cp $(srcdir)/debian/copyright $(distdir)/debian
	mkdir -p $(distdir)/perl
	cd $(srcdir)/perl && ./doswig
	for f in `cat $(srcdir)/perl/MANIFEST`; do \
		(pdir=`cd $(distdir)/perl && pwd`; \
		cd $(srcdir)/perl;cp --parents $$f $$pdir); \
	done

rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba $(SPEC_FILE)
