## $Id: Makefile.am,v 1.14 2004-09-28 12:52:03 adam Exp $

common_libs = libidzebra-recctrl.la \
 ../data1/libidzebra-data1.la \
 ../dfa/libidzebra-dfa.la  \
 ../util/libidzebra-util.la \
 $(YAZLALIB)

# The shared modules 
mod_grs_regx_la_SOURCES = regxread.c
mod_grs_regx_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_grs_regx_la_LADD = $(TCL_LIB)
mod_grs_regx_la_LIBADD = $(common_libs) $(mod_grs_regx_la_LADD)

mod_grs_xml_la_SOURCES = xmlread.c
mod_grs_xml_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_grs_xml_la_LADD = $(EXPAT_LIBS)
mod_grs_xml_la_LIBADD = $(common_libs) $(mod_grs_xml_la_LADD)

mod_grs_marc_la_SOURCES = marcread.c marcomp.c marcomp.h inline.c inline.h
mod_grs_marc_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_grs_marc_la_LADD =
mod_grs_marc_la_LIBADD = $(common_libs) $(mod_grs_marc_la_LADD)

mod_grs_danbib_la_SOURCES = danbibr.c
mod_grs_danbib_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_grs_danbib_la_LADD =
mod_grs_danbib_la_LIBADD = $(common_libs) $(mod_grs_danbib_la_LADD)

pkglib_LTLIBRARIES = $(SHARED_MODULE_LA) 
EXTRA_LTLIBRARIES = \
	mod-grs-regx.la \
	mod-grs-xml.la \
	mod-grs-marc.la  \
	mod-grs-danbib.la

# The common library
lib_LTLIBRARIES = libidzebra-recctrl.la
libidzebra_recctrl_la_SOURCES = recctrl.c recgrs.c sgmlread.c rectext.c 
libidzebra_recctrl_la_LDFLAGS = -export-dynamic
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
 $(mod_grs_danbib_la_SOURCES)

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) $(TCL_INCLUDE) -DDEFAULT_MODULE_PATH=\"$(pkglibdir)\"
