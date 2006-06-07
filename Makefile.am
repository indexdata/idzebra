## $Id: Makefile.am,v 1.14.2.2 2006-06-07 07:43:47 adam Exp $

AUTOMAKE_OPTIONS=foreign

SUBDIRS=util bfile dfa dict isams isamb isamc isam rset data1 \
 recctrl tab index test examples include doc

SPEC_FILE=$(PACKAGE).spec

EXTRA_DIST= README NEWS $(SPEC_FILE) idzebra.spec.in

dist-hook:
	if test -x /usr/bin/cvs2cl -a -d CVS; then cvs2cl -F ZEBRA_1_3_16_branch; fi
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

