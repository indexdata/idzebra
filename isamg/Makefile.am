## $Id: Makefile.am,v 1.1 2001-01-16 19:05:45 heikki Exp $

noinst_LIBRARIES = libisamc.a

libisamc_a_SOURCES = isamg.c 

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a @YAZLIB@
