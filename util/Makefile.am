## $Id: Makefile.am,v 1.8.2.2 2006-10-23 11:37:11 adam Exp $

noinst_LIBRARIES = libutil.a

noinst_PROGRAMS = passtest

check_PROGRAMS = tstflock

EXTRA_DIST = zebrasrv.rh

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"
LDADD = libutil.a $(YAZLIB) $(TCL_LIB)

libutil_a_SOURCES = res.c flock.c charmap.c zebramap.c passwddb.c zebra-lock.c dirent.c xpath.c

passtest_SOURCES = passtest.c

tstflock_SOURCES = tstflock.c
