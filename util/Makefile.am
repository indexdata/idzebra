## $Id: Makefile.am,v 1.12 2004-09-30 08:16:49 adam Exp $

lib_LTLIBRARIES = libidzebra-util.la

noinst_PROGRAMS = passtest

bin_SCRIPTS = idzebra-config

EXTRA_DIST = zebrasrv.rh

DISTCLEANFILES = idzebra-config

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"
LDADD = libidzebra-util.la $(YAZLALIB)

libidzebra_util_la_SOURCES = res.c charmap.c zebramap.c passwddb.c \
 zebra-lock.c dirent.c xpath.c atoi_zn.c

passtest_SOURCES = passtest.c
