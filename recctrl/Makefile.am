## $Id: Makefile.am,v 1.5 2002-11-15 21:26:01 adam Exp $

noinst_LIBRARIES = librecctrl.a

librecctrl_a_SOURCES = recctrl.c recgrs.c sgmlread.c regxread.c \
	marcread.c rectext.c grsread.h recgrs.h rectext.h xmlread.c perlread.c 
INCLUDES = -I$(srcdir)/../include @YAZINC@ $(TCL_INCLUDE)
