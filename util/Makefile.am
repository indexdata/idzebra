## $Id: Makefile.am,v 1.10 2004-08-24 10:37:20 adam Exp $

lib_LTLIBRARIES = libidzebra-util.la

noinst_PROGRAMS = passtest

EXTRA_DIST = zebrasrv.rh

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"
LDADD = libidzebra-util.la $(TCL_LIB) $(YAZLALIB)

libidzebra_util_la_SOURCES = res.c charmap.c zebramap.c passwddb.c \
 zebra-lock.c dirent.c xpath.c atoi_zn.c

passtest_SOURCES = passtest.c
