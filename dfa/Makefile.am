## $Id: Makefile.am,v 1.3 2004-06-16 20:49:17 adam Exp $

noinst_LIBRARIES = libdfa.a

noinst_PROGRAMS = agrep lexer grepper

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)
LDADD = libdfa.a ../util/libutil.a $(YAZLIB) $(TCL_LIB)

agrep_SOURCES = agrep.c

lexer_SOURCES = lexer.c readfile.c

grepper_SOURCES = grepper.c

libdfa_a_SOURCES = dfa.c imalloc.c states.c set.c bset.c \
 dfap.h imalloc.h lexer.h
