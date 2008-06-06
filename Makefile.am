
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
	if test -x /usr/bin/git -a -d .git; then git log >ChangeLog ; cp ChangeLog $(distdir); fi
	cp $(srcdir)/LICENSE* $(distdir)
	mkdir $(distdir)/win
	for i in $(srcdir)/win/*; do \
		if test -f $$i; then \
			cp $$i $(distdir)/win; \
		fi; \
	done

dox:
	doxygen

showdox:
	doxygen
	firefox -new-window file:///`pwd`/$(top_srcdir)/dox/html/index.html &


.PHONY: dox showdox
