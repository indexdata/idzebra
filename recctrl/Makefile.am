## $Id: Makefile.am,v 1.7 2003-02-25 14:18:00 adam Exp $

noinst_LIBRARIES = librecctrl.a

EXTRA_DIST=perlread.c perlread.h.in

if perl
PERLREAD=perlread.c
else
PERLREAD=
endif	

librecctrl_a_SOURCES = recctrl.c recgrs.c sgmlread.c regxread.c \
	marcread.c rectext.c grsread.h recgrs.h rectext.h xmlread.c \
	$(PERLREAD)

INCLUDES = -I$(srcdir)/../include @YAZINC@ $(TCL_INCLUDE)
