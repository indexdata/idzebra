## $Id: Makefile.am,v 1.5 2006-06-29 09:10:41 adam Exp $

zebralibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)

zebralib_LTLIBRARIES = libidzebra-isams.la

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC)

libidzebra_isams_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_isams_la_SOURCES = isams.c
