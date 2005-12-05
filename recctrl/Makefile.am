## $Id: Makefile.am,v 1.18 2005-12-05 12:18:40 marc Exp $

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

mod_grs_csv_la_SOURCES = csvread.c
mod_grs_csv_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_grs_csv_la_LADD =
mod_grs_csv_la_LIBADD = $(common_libs) $(mod_grs_csv_la_LADD)

mod_grs_danbib_la_SOURCES = danbibr.c
mod_grs_danbib_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_grs_danbib_la_LADD =
mod_grs_danbib_la_LIBADD = $(common_libs) $(mod_grs_danbib_la_LADD)

mod_safari_la_SOURCES = safari.c
mod_safari_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_safari_la_LADD =
mod_safari_la_LIBADD = $(common_libs) $(mod_safari_la_LADD)

mod_alvis_la_SOURCES = alvis.c
mod_alvis_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_alvis_la_LADD = $(XSLT_LIBS)
mod_alvis_la_LIBADD = $(common_libs) $(mod_alvis_la_LADD)

mod_xslt_la_SOURCES = xslt.c
mod_xslt_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
mod_xslt_la_LADD = $(XSLT_LIBS)
mod_xslt_la_LIBADD = $(common_libs) $(mod_alvis_la_LADD)

pkglib_LTLIBRARIES = $(SHARED_MODULE_LA) 
EXTRA_LTLIBRARIES = \
	mod-grs-regx.la \
	mod-grs-xml.la \
	mod-grs-marc.la  \
	mod-grs-csv.la  \
	mod-grs-danbib.la \
	mod-safari.la \
	mod-alvis.la \
	mod-xslt.la

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
 $(mod_grs_csv_la_SOURCES) \
 $(mod_grs_danbib_la_SOURCES) \
 $(mod_safari_la_SOURCES) \
 $(mod_alvis_la_SOURCES) \
 $(mod_xslt_la_SOURCES)

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) $(XSLT_CFLAGS) \
 $(TCL_INCLUDE) -DDEFAULT_MODULE_PATH=\"$(pkglibdir)\"
