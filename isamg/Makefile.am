## $Id: Makefile.am,v 1.3 2004-09-30 08:16:49 adam Exp $

noinst_LIBRARIES = libisamc.a

libisamc_a_SOURCES = isamg.c 

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLIB)
