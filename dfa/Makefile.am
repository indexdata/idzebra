## $Id: Makefile.am,v 1.8 2006-06-29 09:10:34 adam Exp $

zebralibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)

zebralib_LTLIBRARIES = libidzebra-dfa.la

noinst_PROGRAMS = agrep lexer grepper

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)
LDADD = libidzebra-dfa.la ../util/libidzebra-util.la $(YAZLALIB)

agrep_SOURCES = agrep.c

lexer_SOURCES = lexer.c readfile.c

grepper_SOURCES = grepper.c

libidzebra_dfa_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_dfa_la_SOURCES = dfa.c imalloc.c states.c set.c bset.c \
 dfap.h imalloc.h lexer.h
