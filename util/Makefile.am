## $Id: Makefile.am,v 1.5 2002-08-17 08:00:28 adam Exp $

noinst_LIBRARIES = libutil.a

noinst_PROGRAMS = passtest

EXTRA_DIST = zebrasrv.rh

INCLUDES = -I$(srcdir)/../include @YAZINC@
LDADD = libutil.a $(YAZLIB) $(TCL_LIB)

libutil_a_SOURCES = res.c charmap.c zebramap.c passwddb.c zebra-lock.c dirent.c

passtest_SOURCES = passtest.c
