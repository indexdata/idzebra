## $Id: Makefile.am,v 1.3 2004-06-16 20:49:18 adam Exp $

noinst_LIBRARIES = libisam.a

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)

LDADD = ../util/libutil.a ../bfile/libbfile.a $(YAZLIB) $(TCL_LIB)

libisam_a_SOURCES = isam.c isutil.c rootblk.c memory.c physical.c \
 isutil.h keyops.h memory.h physical.h rootblk.h
