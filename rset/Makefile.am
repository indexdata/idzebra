## $Id: Makefile.am,v 1.17 2006-07-03 14:27:13 adam Exp $

noinst_LTLIBRARIES = libidzebra-rset.la

libidzebra_rset_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_rset_la_SOURCES = rset.c rstemp.c rsnull.c rsbool.c rsbetween.c \
	rsisamc.c rsmultiandor.c rsisams.c rsisamb.c rsprox.c 

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)
