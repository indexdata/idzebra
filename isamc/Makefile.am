## $Id: Makefile.am,v 1.5 2004-08-04 08:35:24 adam Exp $

noinst_LIBRARIES = libisamc.a

libisamc_a_SOURCES = isamc.c merge.c isamc-p.h

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../index $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLIB) $(TCL_LIB)
