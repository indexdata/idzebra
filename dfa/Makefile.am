## $Id: Makefile.am,v 1.6 2004-09-30 08:16:48 adam Exp $

lib_LTLIBRARIES = libidzebra-dfa.la

noinst_PROGRAMS = agrep lexer grepper

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)
LDADD = libidzebra-dfa.la ../util/libidzebra-util.la $(YAZLALIB)

agrep_SOURCES = agrep.c

lexer_SOURCES = lexer.c readfile.c

grepper_SOURCES = grepper.c

libidzebra_dfa_la_SOURCES = dfa.c imalloc.c states.c set.c bset.c \
 dfap.h imalloc.h lexer.h
