## $Id: Makefile.am,v 1.4 2004-08-24 10:37:18 adam Exp $

lib_LTLIBRARIES = libidzebra-dfa.la

noinst_PROGRAMS = agrep lexer grepper

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)
LDADD = libdfa.a ../util/libutil.a $(YAZLALIB) $(TCL_LIB)

agrep_SOURCES = agrep.c

lexer_SOURCES = lexer.c readfile.c

grepper_SOURCES = grepper.c

libidzebra_dfa_la_SOURCES = dfa.c imalloc.c states.c set.c bset.c \
 dfap.h imalloc.h lexer.h
