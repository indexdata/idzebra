## $Id: Makefile.am,v 1.15 2005-06-15 21:31:45 adam Exp $

lib_LTLIBRARIES = libidzebra-util.la

noinst_PROGRAMS = passtest

check_PROGRAMS = tstcharmap

TESTS = $(check_PROGRAMS)

bin_SCRIPTS = idzebra-config

EXTRA_DIST = zebrasrv.rh tstcharmap.chr

DISTCLEANFILES = idzebra-config

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"
LDADD = libidzebra-util.la $(YAZLALIB)

libidzebra_util_la_SOURCES = zint.c res.c charmap.c zebramap.c passwddb.c \
 zebra-lock.c dirent.c xpath.c atoi_zn.c snippet.c

passtest_SOURCES = passtest.c

tstcharmap_SOURCES = tstcharmap.c
