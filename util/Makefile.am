## $Id: Makefile.am,v 1.7 2003-02-04 12:06:48 pop Exp $

noinst_LIBRARIES = libutil.a

noinst_PROGRAMS = passtest

EXTRA_DIST = zebrasrv.rh

AM_CPPFLAGS = -I$(srcdir)/../include @YAZINC@ -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"
LDADD = libutil.a $(YAZLIB) $(TCL_LIB)

libutil_a_SOURCES = res.c charmap.c zebramap.c passwddb.c zebra-lock.c dirent.c xpath.c

passtest_SOURCES = passtest.c
