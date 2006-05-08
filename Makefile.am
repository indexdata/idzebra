## $Id: Makefile.am,v 1.23 2006-05-08 12:09:49 marc Exp $

AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS = -I m4

SUBDIRS=util bfile dfa dict isams isamb isamc rset data1 \
 recctrl tab index test examples include doc

aclocaldir=$(datadir)/aclocal
aclocal_DATA = m4/idzebra.m4

SPEC_FILE=$(PACKAGE).spec

EXTRA_DIST= README NEWS $(SPEC_FILE) idzebra.spec.in \
 idzebra-config.in m4/idzebra.m4 m4/yaz.m4 Doxyfile.in

dist-hook:
	cp $(srcdir)/LICENSE* $(distdir)
	mkdir $(distdir)/win
	for i in $(srcdir)/win/*; do \
		if test -f $$i; then \
			cp $$i $(distdir)/win; \
		fi; \
	done
	mkdir $(distdir)/debian
	cp $(srcdir)/debian/*.install $(distdir)/debian
	cp $(srcdir)/debian/*.manpages $(distdir)/debian
	cp $(srcdir)/debian/changelog $(distdir)/debian
	cp $(srcdir)/debian/compat $(distdir)/debian
	cp $(srcdir)/debian/control $(distdir)/debian
	cp $(srcdir)/debian/copyright $(distdir)/debian
	cp $(srcdir)/debian/rules $(distdir)/debian

cvs2cl:
	cvs2cl


dox:
	doxygen

showdox:
	doxygen
	firefox -new-window file:///`pwd`/$(top_srcdir)/dox/html/index.html &

