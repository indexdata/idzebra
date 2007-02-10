## $Id: Makefile.am,v 1.30 2007-02-10 12:46:54 adam Exp $

AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS = -I m4

SUBDIRS=util bfile dfa dict isams isamb isamc rset data1 \
 tab index test examples include doc

aclocaldir=$(datadir)/aclocal
aclocal_DATA = m4/idzebra-2.0.m4

SPEC_FILE=idzebra.spec

EXTRA_DIST= README NEWS $(SPEC_FILE) $(SPEC_FILE).in \
 idzebra-config-2.0.in m4/idzebra-2.0.m4 m4/yaz.m4 buildconf.sh Doxyfile.in

dist-hook:
	if test -x /usr/bin/cvs2cl -a -d CVS; then cvs2cl ; fi
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
	cp $(srcdir)/debian/*.postinst $(distdir)/debian
	cp $(srcdir)/debian/*.prerm $(distdir)/debian
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


.PHONY: cvs2cl dox showdox
