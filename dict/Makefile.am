## $Id: Makefile.am,v 1.6 2004-06-16 20:49:18 adam Exp $

noinst_LIBRARIES = libdict.a
noinst_PROGRAMS = dicttest dictext

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)

LDADD = libdict.a ../bfile/libbfile.a ../dfa/libdfa.a ../util/libutil.a $(YAZLIB) $(TCL_LIB)

libdict_a_SOURCES = scan.c dopen.c dclose.c drdwr.c open.c close.c insert.c \
 lookup.c lookupec.c lookgrep.c delete.c dcompact.c

dicttest_SOURCES = dicttest.c

dictext_SOURCES = dictext.c

