## $Id: Makefile.am,v 1.5 2002-09-05 14:36:20 adam Exp $

AUTOMAKE_OPTIONS=foreign

EXTRA_DIST= README CHANGELOG 

SUBDIRS=util bfile dfa dict isams isamb isamc isam rset recctrl tab index test
DIST_SUBDIRS=$(SUBDIRS) include doc

SPEC_FILE=$(PACKAGE).spec

dist-hook:
	cd doc; make
	cp $(srcdir)/LICENSE* $(distdir)
	cp $(srcdir)/*.spec $(distdir)
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


rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba $(SPEC_FILE)
