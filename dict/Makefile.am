## $Id: Makefile.am,v 1.2 2000-07-07 12:49:20 adam Exp $

noinst_LIBRARIES = libdict.a
noinst_PROGRAMS = dicttest dictext

INCLUDES = -I$(srcdir)/../include @YAZINC@
LDADD = libdict.a ../bfile/libbfile.a ../dfa/libdfa.a ../util/libutil.a @YAZLIB@ @LIBS@

libdict_a_SOURCES = scan.c dopen.c dclose.c drdwr.c open.c close.c insert.c \
 lookup.c lookupec.c lookgrep.c delete.c dcompact.c

dicttest_SOURCES = dicttest.c

dictext_SOURCES = dictext.c

