## $Id: Makefile.am,v 1.2 2002-03-21 20:15:57 adam Exp $

noinst_LIBRARIES = libisamc.a

libisamc_a_SOURCES = isamc.c isamd.c merge.c merge-d.c isamc-p.h isamd-p.h

INCLUDES = -I$(srcdir)/../include -I$(srcdir)/../index @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a @YAZLIB@
