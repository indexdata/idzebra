## $Id: Makefile.am,v 1.1 2000-04-05 09:49:36 adam Exp $

noinst_LIBRARIES = libisam.a

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a @YAZLIB@ @LIBS@

libisam_a_SOURCES = isam.c isutil.c rootblk.c memory.c physical.c \
 isutil.h keyops.h memory.h physical.h rootblk.h
