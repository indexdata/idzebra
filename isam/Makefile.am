## $Id: Makefile.am,v 1.2 2002-08-17 08:00:27 adam Exp $

noinst_LIBRARIES = libisam.a

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a $(YAZLIB) $(TCL_LIB)

libisam_a_SOURCES = isam.c isutil.c rootblk.c memory.c physical.c \
 isutil.h keyops.h memory.h physical.h rootblk.h
