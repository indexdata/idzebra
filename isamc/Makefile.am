## $Id: Makefile.am,v 1.1 2000-04-05 09:49:36 adam Exp $

noinst_LIBRARIES = libisamc.a

libisamc_a_SOURCES = isamc.c isamd.c merge.c merge-d.c isamc-p.h isamd-p.h

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a @YAZLIB@
