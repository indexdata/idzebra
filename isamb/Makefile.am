## $Id: Makefile.am,v 1.1 2000-10-11 12:31:27 adam Exp $

noinst_LIBRARIES = libisamb.a

libisamb_a_SOURCES = isamb.c

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a @YAZLIB@
