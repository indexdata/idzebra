
AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS = -I m4

SUBDIRS=util bfile dfa dict isams isamb isamc rset data1 \
 tab index test examples include doc

aclocaldir=$(datadir)/aclocal
aclocal_DATA = m4/idzebra-2.0.m4

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = zebra.pc

SPEC_FILE=idzebra.spec

EXTRA_DIST= README.md NEWS IDMETA $(SPEC_FILE) m4/id-config.sh \
 idzebra-config-2.0.in m4/idzebra-2.0.m4 m4/yaz.m4 buildconf.sh Doxyfile.in \
 m4/common.nsi m4/mk_version.tcl

dist-hook:
	if test -x /usr/bin/git -a -d .git; then git log >ChangeLog ; cp ChangeLog $(distdir); fi
	cp $(srcdir)/LICENSE* $(distdir)
	test -d $(distdir)/win || mkdir $(distdir)/win
	for i in $(srcdir)/win/*; do \
		if test -f $$i; then \
			cp $$i $(distdir)/win; \
		fi; \
	done
	mkdir $(distdir)/rpm
	-cp $(srcdir)/rpm/* $(distdir)/rpm

dox:
	doxygen

showdox:
	doxygen
	firefox -new-window file:///`pwd`/$(top_srcdir)/dox/html/index.html &


.PHONY: dox showdox
