## $Id: Makefile.am,v 1.65 2007-11-23 13:52:52 adam Exp $

aux_libs = \
 ../rset/libidzebra-rset.la \
 ../dict/libidzebra-dict.la  \
 ../isams/libidzebra-isams.la \
 ../isamc/libidzebra-isamc.la \
 ../isamb/libidzebra-isamb.la \
 ../data1/libidzebra-data1.la \
 ../bfile/libidzebra-bfile.la \
 ../dfa/libidzebra-dfa.la \
 ../util/libidzebra-util.la

# Should match the main_zebralib in configure.ac
# Should also match the libidzebra_..._la lines..!
zebralib = libidzebra-2.0.la

noinst_PROGRAMS = kdump zebrash

modlibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)/modules
tabdatadir = $(datadir)/$(PACKAGE)$(PACKAGE_SUFFIX)/tab

# The shared modules 
mod_grs_regx_la_SOURCES = regxread.c
mod_grs_regx_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_regx_la_LADD = $(TCL_LIB)
mod_grs_regx_la_LIBADD = $(zebralib) $(mod_grs_regx_la_LADD)

mod_grs_xml_la_SOURCES = xmlread.c
mod_grs_xml_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_xml_la_LADD = $(EXPAT_LIBS)
mod_grs_xml_la_LIBADD = $(zebralib) $(mod_grs_xml_la_LADD)

mod_grs_marc_la_SOURCES = marcread.c marcomp.c marcomp.h inline.c inline.h
mod_grs_marc_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_marc_la_LADD =
mod_grs_marc_la_LIBADD = $(zebralib) $(mod_grs_marc_la_LADD)

mod_safari_la_SOURCES = safari.c
mod_safari_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_safari_la_LADD =
mod_safari_la_LIBADD = $(zebralib) $(mod_safari_la_LADD)

mod_alvis_la_SOURCES = alvis.c
mod_alvis_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_alvis_la_LADD =
mod_alvis_la_LIBADD = $(zebralib) $(mod_alvis_la_LADD)

mod_dom_la_SOURCES = mod_dom.c
mod_dom_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_dom_la_LADD =
mod_dom_la_LIBADD = $(zebralib) $(mod_dom_la_LADD)

mod_text_la_SOURCES = rectext.c
mod_text_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_text_la_LADD =
mod_text_la_LIBADD = $(zebralib) $(mod_text_la_LADD)

modlib_LTLIBRARIES = $(SHARED_MODULE_LA) 
EXTRA_LTLIBRARIES = \
	mod-grs-regx.la \
	mod-grs-xml.la \
	mod-grs-marc.la  \
	mod-safari.la \
	mod-alvis.la \
	mod-dom.la \
	mod-text.la

EXTRA_libidzebra_2_0_la_SOURCES = \
 $(mod_grs_regx_la_SOURCES) \
 $(mod_grs_xml_la_SOURCES) \
 $(mod_grs_marc_la_SOURCES) \
 $(mod_safari_la_SOURCES) \
 $(mod_alvis_la_SOURCES) \
 $(mod_dom_la_SOURCES) \
 $(mod_text_la_SOURCES)

lib_LTLIBRARIES = $(zebralib)

libidzebra_2_0_la_DEPENDENCIES = $(STATIC_MODULE_OBJ) $(aux_libs)
libidzebra_2_0_la_LIBADD = $(STATIC_MODULE_OBJ)  $(aux_libs) $(STATIC_MODULE_LADD)
libidzebra_2_0_la_LDFLAGS=-export-dynamic -version-info $(ZEBRALIBS_VERSION_INFO)
libidzebra_2_0_la_SOURCES = \
  recctrl.c \
  recgrs.c \
  sgmlread.c \
  attribute.c \
  compact.c \
  dir.c dirs.c \
  extract.c \
  index.h invstat.c \
  kinput.c kcontrol.c \
  limit.c \
  orddict.c orddict.h \
  rank.h rank1.c ranksimilarity.c rankstatic.c \
  records.c recindex.c recindex.h recindxp.h reckeys.c reckeys.h \
  recstat.c retrieve.c \
  rpnscan.c rpnsearch.c rpnfacet.c sortidx.c symtab.c stream.c \
  update_path.c update_file.c trunc.c untrans.c isam_methods.c \
  zaptterm.c zebraapi.c zinfo.c zinfo.h zsets.c key_block.c key_block.h \
  check_res.c

bin_PROGRAMS = zebraidx zebrasrv

zebraidx_SOURCES = zebraidx.c
zebrasrv_SOURCES = zebrasrv.c
zebrash_SOURCES = zebrash.c
kdump_SOURCES = kdump.c

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) \
  -DDEFAULT_PROFILE_PATH=\"$(tabdatadir)\" \
  -DDEFAULT_MODULE_PATH=\"$(modlibdir)\" \
  $(TCL_INCLUDE)

LDADD = $(zebralib) $(YAZLALIB) 

zebrash_LDADD= $(LDADD) $(READLINE_LIBS)

install-exec-hook:
	cd $(DESTDIR)$(bindir); for p in $(bin_PROGRAMS); do \
		mv -f $$p $$p$(PACKAGE_SUFFIX); \
		ln -sf $$p$(PACKAGE_SUFFIX) $$p; \
	done

uninstall-hook:
	cd $(DESTDIR)$(bindir) && for p in $(bin_PROGRAMS); do \
		rm -f $$p$(PACKAGE_SUFFIX); \
	done

