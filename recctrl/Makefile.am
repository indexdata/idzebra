## $Id: Makefile.am,v 1.8 2003-02-28 12:33:39 oleg Exp $

noinst_LIBRARIES = librecctrl.a

EXTRA_DIST=perlread.c perlread.h.in

if perl
PERLREAD=perlread.c
else
PERLREAD=
endif	

librecctrl_a_SOURCES = recctrl.c recgrs.c sgmlread.c regxread.c \
	marcread.c marcomp.c marcomp.h inline.c inline.h rectext.c \
	grsread.h recgrs.h rectext.h xmlread.c \
	$(PERLREAD)

INCLUDES = -I$(srcdir)/../include @YAZINC@ $(TCL_INCLUDE)
