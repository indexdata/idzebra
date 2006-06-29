## $Id: Makefile.am,v 1.25 2006-06-29 09:10:42 adam Exp $

zebralibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)

modlibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)/modules

common_libs = libidzebra-recctrl.la \
 ../data1/libidzebra-data1.la \
 ../dfa/libidzebra-dfa.la  \
 ../util/libidzebra-util.la \
 $(YAZLALIB)

# The shared modules 
mod_grs_regx_la_SOURCES = regxread.c
mod_grs_regx_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_regx_la_LADD = $(TCL_LIB)
mod_grs_regx_la_LIBADD = $(common_libs) $(mod_grs_regx_la_LADD)

mod_grs_xml_la_SOURCES = xmlread.c
mod_grs_xml_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_xml_la_LADD = $(EXPAT_LIBS)
mod_grs_xml_la_LIBADD = $(common_libs) $(mod_grs_xml_la_LADD)

mod_grs_marc_la_SOURCES = marcread.c marcomp.c marcomp.h inline.c inline.h
mod_grs_marc_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_marc_la_LADD =
mod_grs_marc_la_LIBADD = $(common_libs) $(mod_grs_marc_la_LADD)

mod_safari_la_SOURCES = safari.c
mod_safari_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_safari_la_LADD =
mod_safari_la_LIBADD = $(common_libs) $(mod_safari_la_LADD)

mod_alvis_la_SOURCES = alvis.c
mod_alvis_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_alvis_la_LADD = $(XSLT_LIBS)
mod_alvis_la_LIBADD = $(common_libs) $(mod_alvis_la_LADD)

modlib_LTLIBRARIES = $(SHARED_MODULE_LA) 
EXTRA_LTLIBRARIES = \
	mod-grs-regx.la \
	mod-grs-xml.la \
	mod-grs-marc.la  \
	mod-safari.la \
	mod-alvis.la

# The common library
zebralib_LTLIBRARIES = libidzebra-recctrl.la
libidzebra_recctrl_la_SOURCES = recctrl.c recgrs.c sgmlread.c rectext.c 
libidzebra_recctrl_la_LDFLAGS = -export-dynamic -version-info $(ZEBRALIBS_VERSION)
libidzebra_recctrl_la_LIBADD = $(STATIC_MODULE_OBJ) \
 ../data1/libidzebra-data1.la \
 ../dfa/libidzebra-dfa.la  \
 ../util/libidzebra-util.la \
 $(STATIC_MODULE_LADD)

libidzebra_recctrl_la_DEPENDENCIES = $(STATIC_MODULE_OBJ)

EXTRA_libidzebra_recctrl_la_SOURCES = \
 $(mod_grs_regx_la_SOURCES) \
 $(mod_grs_xml_la_SOURCES) \
 $(mod_grs_marc_la_SOURCES) \
 $(mod_safari_la_SOURCES) \
 $(mod_alvis_la_SOURCES)

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) $(XSLT_CFLAGS) $(TCL_INCLUDE) 
