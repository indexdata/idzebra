## $Id: Makefile.am,v 1.3 2002-08-17 08:00:28 adam Exp $

noinst_LIBRARIES = libisamc.a

libisamc_a_SOURCES = isamc.c isamd.c merge.c merge-d.c isamc-p.h isamd-p.h

INCLUDES = -I$(srcdir)/../include -I$(srcdir)/../index @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLIB) $(TCL_LIB)
