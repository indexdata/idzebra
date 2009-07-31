
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
mod_grs_regx_la_SOURCES = mod_grs_regx.c
mod_grs_regx_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_regx_la_LADD = $(TCL_LIB)
mod_grs_regx_la_LIBADD = $(zebralib) $(mod_grs_regx_la_LADD)

mod_grs_xml_la_SOURCES = mod_grs_xml.c
mod_grs_xml_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_xml_la_LADD = $(EXPAT_LIBS)
mod_grs_xml_la_LIBADD = $(zebralib) $(mod_grs_xml_la_LADD)

mod_grs_marc_la_SOURCES = mod_grs_marc.c marcomp.c marcomp.h inline.c inline.h
mod_grs_marc_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_grs_marc_la_LADD =
mod_grs_marc_la_LIBADD = $(zebralib) $(mod_grs_marc_la_LADD)

mod_safari_la_SOURCES = mod_safari.c
mod_safari_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_safari_la_LADD =
mod_safari_la_LIBADD = $(zebralib) $(mod_safari_la_LADD)

mod_alvis_la_SOURCES = mod_alvis.c
mod_alvis_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_alvis_la_LADD =
mod_alvis_la_LIBADD = $(zebralib) $(mod_alvis_la_LADD)

mod_dom_la_SOURCES = mod_dom.c
mod_dom_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_dom_la_LADD =
mod_dom_la_LIBADD = $(zebralib) $(mod_dom_la_LADD)

mod_text_la_SOURCES = mod_text.c
mod_text_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_text_la_LADD =
mod_text_la_LIBADD = $(zebralib) $(mod_text_la_LADD)

mod_indexplugin_mysql_la_SOURCES = mod_indexplugin_mysql.c
mod_indexplugin_mysql_la_LDFLAGS = -rpath $(modlibdir) -module -avoid-version
mod_indexplugin_mysql_la_LADD =
mod_indexplugin_mysql_la_LIBADD = $(zebralib) $(mod_indexplugin_mysql_la_LADD) -lmysqlclient

modlib_LTLIBRARIES = $(SHARED_MODULE_LA) 
EXTRA_LTLIBRARIES = \
	mod-grs-regx.la \
	mod-grs-xml.la \
	mod-grs-marc.la  \
	mod-safari.la \
	mod-alvis.la \
	mod-dom.la \
	mod-text.la \
	mod-indexplugin_mysql.la

EXTRA_libidzebra_2_0_la_SOURCES = \
 $(mod_grs_regx_la_SOURCES) \
 $(mod_grs_xml_la_SOURCES) \
 $(mod_grs_marc_la_SOURCES) \
 $(mod_safari_la_SOURCES) \
 $(mod_alvis_la_SOURCES) \
 $(mod_dom_la_SOURCES) \
 $(mod_text_la_SOURCES) \
 $(mod_indexplugin_mysql_la_SOURCES)

lib_LTLIBRARIES = $(zebralib)

libidzebra_2_0_la_DEPENDENCIES = $(STATIC_MODULE_OBJ) $(aux_libs)
libidzebra_2_0_la_LIBADD = $(STATIC_MODULE_OBJ)  $(aux_libs) $(STATIC_MODULE_LADD)
libidzebra_2_0_la_LDFLAGS=-export-dynamic -version-info $(ZEBRALIBS_VERSION_INFO)
libidzebra_2_0_la_SOURCES = \
  recctrl.c \
  recgrs.c \
  mod_grs_sgml.c \
  attribute.c \
  compact.c \
  dir.c dirs.c \
  extract.c \
  index.h invstat.c \
  kinput.c kcontrol.c \
  limit.c \
  orddict.c orddict.h \
  rank.h rank1.c ranksimilarity.c rankstatic.c \
  records.c recindex.c recindex.h reckeys.c reckeys.h \
  retrieve.c \
  rpnscan.c rpnsearch.c sortidx.c stream.c \
  update_path.c update_file.c trunc.c untrans.c isam_methods.c \
  zaptterm.c zebraapi.c zinfo.c zinfo.h zsets.c key_block.c key_block.h \
  check_res.c rset_isam.c \
  update_driver.c

bin_PROGRAMS = zebraidx zebrasrv

zebraidx_SOURCES = zebraidx.c
zebrasrv_SOURCES = zebrasrv.c
zebrash_SOURCES = zebrash.c
kdump_SOURCES = kdump.c

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) \
  -DDEFAULT_PROFILE_PATH=\"$(tabdatadir)\" \
  -DDEFAULT_MODULE_PATH=\"$(modlibdir)\" \
  $(TCL_INCLUDE)

LDADD = $(zebralib) $(YAZLALIB) -ldl 

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

