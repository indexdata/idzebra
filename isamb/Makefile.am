## $Id: Makefile.am,v 1.2 2002-08-17 08:00:28 adam Exp $

noinst_LIBRARIES = libisamb.a

libisamb_a_SOURCES = isamb.c

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLIB) $(TCL_LIB)
