## $Id: Makefile.am,v 1.1 2000-04-05 09:49:35 adam Exp $

AUTOMAKE_OPTIONS=foreign

EXTRA_DIST= README CHANGELOG 

if ISZMBOL
SUBDIRS=util bfile dfa dict isams isamc isam rset recctrl tab index test
else
SUBDIRS=util bfile dfa dict isams rset recctrl tab index test
endif
DIST_SUBDIRS=$(SUBDIRS) include doc

SPEC_FILE=$(PACKAGE).spec

dist-hook:
	cd doc; make
	cp $(srcdir)/LICENSE* $(distdir)
	mkdir $(distdir)/win
	for i in $(srcdir)/win/*; do \
		if test -f $$i; then \
			cp $$i $(distdir)/win; \
		fi; \
	done

rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba $(SPEC_FILE)
