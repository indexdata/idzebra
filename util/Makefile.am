## $Id: Makefile.am,v 1.1 2000-04-05 09:49:36 adam Exp $

noinst_LIBRARIES = libutil.a

noinst_PROGRAMS = passtest

INCLUDES = -I$(srcdir)/../include @YAZINC@
LDADD = libutil.a @YAZLIB@ @LIBS@

TESTS = passtest

libutil_a_SOURCES = res.c charmap.c zebramap.c passwddb.c zebra-lock.c

passtest_SOURCES = passtest.c


LIBS =
