## $Id: Makefile.am,v 1.11 2003-02-20 21:06:47 adam Exp $

AUTOMAKE_OPTIONS=foreign

SUBDIRS=util bfile dfa dict isams isamb isamc isam rset data1 \
 recctrl tab index test examples include doc

SPEC_FILE=$(PACKAGE).spec

EXTRA_DIST= README CHANGELOG $(SPEC_FILE) idzebra.spec.in

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
	mkdir $(distdir)/perl
	for f in `cat $(srcdir)/perl/MANIFEST`; do \
		(pdir=`cd $(distdir)/perl && pwd`; \
		cd $(srcdir)/perl;cp --parents $$f $$pdir); \
	done

rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba $(SPEC_FILE)
