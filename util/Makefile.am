## $Id: Makefile.am,v 1.3 2001-09-21 10:40:08 adam Exp $

noinst_LIBRARIES = libutil.a

noinst_PROGRAMS = passtest

EXTRA_DIST = zebrasrv.rh

INCLUDES = -I$(srcdir)/../include @YAZINC@
LDADD = libutil.a @YAZLIB@ @LIBS@

TESTS = passtest

libutil_a_SOURCES = res.c charmap.c zebramap.c passwddb.c zebra-lock.c dirent.c

passtest_SOURCES = passtest.c


LIBS =
