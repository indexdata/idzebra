
noinst_LTLIBRARIES = libidzebra-dfa.la

noinst_PROGRAMS = agrep lexer grepper

check_PROGRAMS = test_dfa
TESTS = $(check_PROGRAMS)

test_dfa_SOURCES = test_dfa.c

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZ_CFLAGS)
LDADD = libidzebra-dfa.la ../util/libidzebra-util.la $(YAZ_LIBS)

agrep_SOURCES = agrep.c

lexer_SOURCES = lexer.c readfile.c

grepper_SOURCES = grepper.c

libidzebra_dfa_la_SOURCES = dfa.c imalloc.c states.c set.c bset.c \
 dfap.h imalloc.h lexer.h

clean-local:
	-rm -f *.log
