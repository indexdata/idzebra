## $Id: Makefile.am,v 1.9 2004-08-04 08:35:27 adam Exp $

noinst_LIBRARIES = libutil.a

noinst_PROGRAMS = passtest

EXTRA_DIST = zebrasrv.rh

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"
LDADD = libutil.a $(YAZLIB) $(TCL_LIB)

libutil_a_SOURCES = res.c charmap.c zebramap.c passwddb.c zebra-lock.c dirent.c xpath.c atoi_zn.c

passtest_SOURCES = passtest.c
