## $Id: Makefile.am,v 1.4 2004-06-16 20:49:18 adam Exp $

noinst_LIBRARIES = libisamc.a

libisamc_a_SOURCES = isamc.c isamd.c merge.c merge-d.c isamc-p.h isamd-p.h

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../index $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLIB) $(TCL_LIB)
