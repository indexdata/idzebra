## $Id: Makefile.am,v 1.4 2002-05-07 11:04:58 adam Exp $

noinst_LIBRARIES = libutil.a

noinst_PROGRAMS = passtest

EXTRA_DIST = zebrasrv.rh

INCLUDES = -I$(srcdir)/../include @YAZINC@
LDADD = libutil.a @YAZLIB@ @LIBS@

libutil_a_SOURCES = res.c charmap.c zebramap.c passwddb.c zebra-lock.c dirent.c

passtest_SOURCES = passtest.c

LIBS =
