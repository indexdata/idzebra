
noinst_LTLIBRARIES = libidzebra-util.la

check_PROGRAMS = tstcharmap tstflock tstlockscope tstpass tstres test_strmap

TESTS = $(check_PROGRAMS)

bin_SCRIPTS = idzebra-config-2.0 idzebra-abs2dom

EXTRA_DIST = tstcharmap.chr emptycharmap.chr tstpass.txt tstres.cfg idzebra-abs2dom

DISTCLEANFILES = idzebra-config-2.0

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZ_CFLAGS) -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"
LDADD = libidzebra-util.la $(YAZ_LIBS)

libidzebra_util_la_SOURCES = version.c zint.c res.c charmap.c zebramap.c \
 passwddb.c zebra-lock.c dirent.c xpath.c atoi_zn.c snippet.c flock.c \
 attrfind.c exit.c it_key.c su_codec.c strmap.c

tstpass_SOURCES = tstpass.c

tstcharmap_SOURCES = tstcharmap.c

tstflock_SOURCES = tstflock.c

tstlockscope_SOURCES = tstlockscope.c

tstres_SOURCES = tstres.c

test_strmap_SOURCES = test_strmap.c

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
	-rm -rf tstflock.out

