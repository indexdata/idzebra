## $Id: Makefile.am,v 1.11 2004-08-24 10:37:18 adam Exp $

lib_LTLIBRARIES = libidzebra-recctrl.la

EXTRA_DIST=perlread.c perlread.h.in

if perl
PERLREAD=perlread.c
else
PERLREAD=
endif	

libidzebra_recctrl_la_SOURCES = recctrl.c recgrs.c sgmlread.c regxread.c \
	marcread.c marcomp.c marcomp.h inline.c inline.h rectext.c \
	grsread.h recgrs.h rectext.h xmlread.c danbibr.c \
	$(PERLREAD)

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) $(TCL_INCLUDE)
