## $Id: Makefile.am,v 1.54 2006-09-21 08:56:52 adam Exp $

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

noinst_PROGRAMS = apitest kdump zebrash

modlibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)/modules

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
mod_alvis_la_LADD = $(XSLT_LIBS)
mod_alvis_la_LIBADD = $(zebralib) $(mod_alvis_la_LADD)

modlib_LTLIBRARIES = $(SHARED_MODULE_LA) 
EXTRA_LTLIBRARIES = \
	mod-grs-regx.la \
	mod-grs-xml.la \
	mod-grs-marc.la  \
	mod-safari.la \
	mod-alvis.la

EXTRA_libidzebra_2_0_la_SOURCES = \
 $(mod_grs_regx_la_SOURCES) \
 $(mod_grs_xml_la_SOURCES) \
 $(mod_grs_marc_la_SOURCES) \
 $(mod_safari_la_SOURCES) \
 $(mod_alvis_la_SOURCES)

lib_LTLIBRARIES = $(zebralib)

libidzebra_2_0_la_DEPENDENCIES = $(STATIC_MODULE_OBJ) $(aux_libs)
libidzebra_2_0_la_LIBADD = $(STATIC_MODULE_OBJ)  $(aux_libs) $(STATIC_MODULE_LADD)
libidzebra_2_0_la_LDFLAGS=-export-dynamic -version-info $(ZEBRALIBS_VERSION_INFO)
libidzebra_2_0_la_SOURCES = \
  recctrl.c \
  recgrs.c \
  sgmlread.c \
  rectext.c \
  attribute.c \
  compact.c \
  dir.c dirs.c \
  extract.c \
  index.h invstat.c \
  kinput.c kcompare.c  kcontrol.c \
  limit.c \
  orddict.c orddict.h \
  rank.h rank1.c ranksimilarity.c rankstatic.c \
  recindex.c  recindex.h recindxp.h reckeys.c reckeys.h recstat.c retrieve.c \
  rpnscan.c rpnsearch.c sortidx.c symtab.c stream.c \
  update_path.c update_file.c trunc.c untrans.c \
  zaptterm.c zebraapi.c zinfo.c zinfo.h zserver.h zsets.c 

bin_PROGRAMS = zebraidx zebrasrv

zebraidx_SOURCES = main.c
zebrasrv_SOURCES = zserver.c
zebrash_SOURCES = zebrash.c
apitest_SOURCES = apitest.c
kdump_SOURCES = kdump.c

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) \
  -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\" \
  $(TCL_INCLUDE) $(XSLT_FLAGS) $(XSLT_CFLAGS)

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

