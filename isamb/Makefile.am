## $Id: Makefile.am,v 1.3 2003-06-23 15:36:11 adam Exp $

noinst_LIBRARIES = libisamb.a

check_PROGRAMS = tstisamb

TESTS = $(check_PROGRAMS)

tstisamb_SOURCES = tstisamb.c
tstisamb_LDADD = libisamb.a ../bfile/libbfile.a ../util/libutil.a $(YAZLIB)

libisamb_a_SOURCES = isamb.c

INCLUDES = -I$(srcdir)/../include @YAZINC@

LDADD = ../util/libutil.a ../bfile/libbfile.a libisamc.a $(YAZLIB) $(TCL_LIB)
