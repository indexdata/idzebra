## $Id: Makefile.am,v 1.2 2002-08-17 08:00:27 adam Exp $

noinst_LIBRARIES = libdfa.a

noinst_PROGRAMS = agrep lexer grepper

INCLUDES = -I$(srcdir)/../include @YAZINC@
LDADD = libdfa.a ../util/libutil.a $(YAZLIB) $(TCL_LIB)

agrep_SOURCES = agrep.c

lexer_SOURCES = lexer.c readfile.c

grepper_SOURCES = grepper.c

libdfa_a_SOURCES = dfa.c imalloc.c states.c set.c bset.c \
 dfap.h imalloc.h lexer.h
