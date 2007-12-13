## $Id: Makefile.am,v 1.13 2007-12-13 20:19:40 adam Exp $

noinst_LTLIBRARIES = libidzebra-dfa.la

noinst_PROGRAMS = agrep lexer grepper

check_PROGRAMS = test_dfa
TESTS = $(check_PROGRAMS)

test_dfa_SOURCES = test_dfa.c

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)
LDADD = libidzebra-dfa.la ../util/libidzebra-util.la $(YAZLALIB)

agrep_SOURCES = agrep.c

lexer_SOURCES = lexer.c readfile.c

grepper_SOURCES = grepper.c

libidzebra_dfa_la_SOURCES = dfa.c imalloc.c states.c set.c bset.c \
 dfap.h imalloc.h lexer.h

clean-local:
	-rm -f *.log
