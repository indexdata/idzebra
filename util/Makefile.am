## $Id: Makefile.am,v 1.2 2001-07-09 23:12:08 adam Exp $

noinst_LIBRARIES = libutil.a

noinst_PROGRAMS = passtest

INCLUDES = -I$(srcdir)/../include @YAZINC@
LDADD = libutil.a @YAZLIB@ @LIBS@

TESTS = passtest

libutil_a_SOURCES = res.c charmap.c zebramap.c passwddb.c zebra-lock.c dirent.c

passtest_SOURCES = passtest.c


LIBS =
