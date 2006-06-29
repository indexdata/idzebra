## $Id: Makefile.am,v 1.16 2006-06-29 13:48:05 adam Exp $

lib_LTLIBRARIES = libidzebra-rset.la

libidzebra_rset_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_rset_la_SOURCES = rset.c rstemp.c rsnull.c rsbool.c rsbetween.c \
	rsisamc.c rsmultiandor.c rsisams.c rsisamb.c rsprox.c 

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)
