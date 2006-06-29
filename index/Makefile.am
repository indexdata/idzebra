## $Id: Makefile.am,v 1.46 2006-06-29 09:10:37 adam Exp $

zebralibdir=$(libdir)/$(PACKAGE)$(PACKAGE_SUFFIX)

noinst_PROGRAMS = apitest kdump zebrash

zebralib_LTLIBRARIES = libidzebra-api.la

libidzebra_api_la_LDFLAGS=-version-info $(ZEBRALIBS_VERSION)
libidzebra_api_la_SOURCES = \
  api_swig.c attribute.c \
  compact.c \
  dir.c dirs.c \
  extract.c \
  index.h invstat.c \
  kinput.c kcompare.c  kcontrol.c \
  limit.c \
  orddict.c orddict.h \
  rank.h rank1.c ranksimilarity.c rankstatic.c \
  recindex.c  recindex.h recindxp.h reckeys.c reckeys.h recstat.c retrieve.c \
  sortidx.c symtab.c \
  update_path.c update_file.c trunc.c \
  zebraapi.c zinfo.c zinfo.h zserver.h zsets.c zrpn.c


bin_PROGRAMS = zebraidx zebrasrv

zebralibs = \
 libidzebra-api.la \
 ../rset/libidzebra-rset.la \
 ../recctrl/libidzebra-recctrl.la \
 ../dict/libidzebra-dict.la  \
 ../isams/libidzebra-isams.la \
 ../isamc/libidzebra-isamc.la \
 ../isamb/libidzebra-isamb.la \
 ../data1/libidzebra-data1.la \
 ../bfile/libidzebra-bfile.la \
 ../dfa/libidzebra-dfa.la \
 ../util/libidzebra-util.la

zebraidx_SOURCES = main.c
zebrasrv_SOURCES = zserver.c
zebrash_SOURCES = zebrash.c
apitest_SOURCES = apitest.c
kdump_SOURCES = kdump.c

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) \
  -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"

zebrash_LDADD= $(zebralibs) $(YAZLALIB) $(READLINE_LIBS)

LDADD = $(zebralibs) $(YAZLALIB)

install-exec-hook:
	cd $(DESTDIR)$(bindir); for p in $(bin_PROGRAMS); do \
		mv -f $$p $$p$(PACKAGE_SUFFIX); \
		ln -sf $$p$(PACKAGE_SUFFIX) $$p; \
	done

uninstall-hook:
	cd $(DESTDIR)$(bindir) && for p in $(bin_PROGRAMS); do \
		rm -f $$p$(PACKAGE_SUFFIX); \
	done

