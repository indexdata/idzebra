## $Id: Makefile.am,v 1.35 2007-10-24 13:55:55 adam Exp $

noinst_LTLIBRARIES = libidzebra-util.la

check_PROGRAMS = tstcharmap tstflock tstlockscope tstpass tstres \
	tst_index_rules

TESTS = $(check_PROGRAMS)

bin_SCRIPTS = idzebra-config-2.0

EXTRA_DIST = zebrasrv.rh tstcharmap.chr tstpass.txt tstres.cfg

DISTCLEANFILES = idzebra-config-2.0

AM_CPPFLAGS = -I$(srcdir)/../include $(YAZINC) -DDEFAULT_PROFILE_PATH=\"$(pkgdatadir)/tab\"
LDADD = libidzebra-util.la $(YAZLALIB)

libidzebra_util_la_SOURCES = version.c zint.c res.c charmap.c zebramap.c \
 passwddb.c zebra-lock.c dirent.c xpath.c atoi_zn.c snippet.c flock.c \
 attrfind.c exit.c it_key.c su_codec.c index_rules.c 

tstpass_SOURCES = tstpass.c

tstcharmap_SOURCES = tstcharmap.c

tstflock_SOURCES = tstflock.c

tstlockscope_SOURCES = tstlockscope.c

tstres_SOURCES = tstres.c

tst_index_rules_SOURCES = tst_index_rules.c

clean-local:
	-rm -rf *.LCK 
	-rm -rf *.log 
	-rm -rf *.mf 
	-rm -rf tstflock.out

