## $Id: Makefile.am,v 1.6 2002-11-26 21:45:29 adam Exp $

noinst_LIBRARIES = librecctrl.a

if perl
PERLREAD=perlread.c
else
PERLREAD=
endif	

librecctrl_a_SOURCES = recctrl.c recgrs.c sgmlread.c regxread.c \
	marcread.c rectext.c grsread.h recgrs.h rectext.h xmlread.c \
	$(PERLREAD)

INCLUDES = -I$(srcdir)/../include @YAZINC@ $(TCL_INCLUDE)
