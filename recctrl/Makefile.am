## $Id: Makefile.am,v 1.9 2004-05-21 11:58:56 adam Exp $

noinst_LIBRARIES = librecctrl.a

EXTRA_DIST=perlread.c perlread.h.in

if perl
PERLREAD=perlread.c
else
PERLREAD=
endif	

librecctrl_a_SOURCES = recctrl.c recgrs.c sgmlread.c regxread.c \
	marcread.c marcomp.c marcomp.h inline.c inline.h rectext.c \
	grsread.h recgrs.h rectext.h xmlread.c danbibr.c \
	$(PERLREAD)

INCLUDES = -I$(srcdir)/../include @YAZINC@ $(TCL_INCLUDE)
