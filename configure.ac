dnl Zebra, Index Data ApS, 1995-2006
dnl $Id: configure.ac,v 1.4 2006-04-24 13:23:15 marc Exp $
dnl
AC_PREREQ(2.59)
AC_INIT([idzebra],[1.4.0],[adam@indexdata.dk])
AC_CONFIG_SRCDIR(configure.ac)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([1.8])
dnl ------ Substitutions
AC_SUBST(TCL_INCLUDE)
AC_SUBST(TCL_LIB)
AC_SUBST(READLINE_LIBS)
dnl
dnl ------ Checking programs
AC_PROG_CC
AC_PROG_CPP
AM_DISABLE_SHARED
AM_PROG_LIBTOOL
dnl
dnl ------ headers
AC_CHECK_HEADERS(sys/resource.h sys/times.h sys/time.h sys/wait.h unistd.h)
dnl ------ crypt
AC_CHECK_LIB(crypt, crypt)
if test "$ac_cv_lib_crypt_crypt" = "yes"; then
	AC_CHECK_HEADERS(crypt.h)
fi
dnl
dnl ------ threads
AC_ARG_ENABLE(threads, [  --disable-threads       disable threads],[enable_threads=$enableval],[enable_threads=yes])
if test "$enable_threads" = "yes"; then
	yazflag=threads
else
	yazflag=""
fi
dnl
dnl ------ YAZ
YAZ_INIT($yazflag,2.1.9)
YAZ_DOC
dnl ----- libXSLT
AC_SUBST(XSLT_LIBS)
AC_SUBST(XSLT_CFLAGS)
xsltdir=yes
AC_ARG_WITH(xslt,[[  --with-xslt[=PREFIX]    use libxslt in PREFIX]],xsltdir=$withval)
if test "$xsltdir" = "yes"; then
	for d in /usr /usr/local; do
		if test -x $d/bin/xslt-config; then
			xsltdir=$d
		fi
	done
fi
if test "$xsltdir" != "no"; then
	AC_MSG_CHECKING(for libXSLT)

	if test -x $xsltdir/bin/xslt-config; then
		XSLT_VER=`$xsltdir/bin/xslt-config --version`
		AC_MSG_RESULT($XSLT_VER)
		have_xslt_version=`echo "$XSLT_VER" | awk 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
		if test $have_xslt_version -lt 1001011; then
			AC_MSG_WARN([libXSLT too old. Use 1.1.11 or later.])
			XSLT_VER=""
		else
			XSLT_LIBS=`$xsltdir/bin/xslt-config --libs`
			XSLT_CFLAGS=`$xsltdir/bin/xslt-config --cflags`
			AC_DEFINE(HAVE_XSLT)
		fi
	else
		AC_MSG_RESULT(Not found)
	fi
fi
dnl ------ Look for Tcl
dnl See if user has specified location of tclConfig.sh; otherwise
dnl see if tclConfig.sh exists in same prefix lcoation as tclsh; otherwise
dnl disable Tcl.
TCL_LIB=""
TCL_INCLUDE=""
tclconfig=NONE
AC_ARG_WITH(tclconfig, [  --with-tclconfig=DIR    tclConfig.sh in DIR], [tclconfig=$withval])
if test "x$tclconfig" = xNONE; then
	saveprefix=${prefix}
	AC_PREFIX_PROGRAM(tclsh)
	tclconfig=${prefix}/lib
	prefix=${saveprefix}
	if test ! -r ${tclconfig}/tclConfig.sh; then
		# Not found, try search for Tcl on Debian systems.
		for d in /usr/lib/tcl*; do
			if test -f $d/tclConfig.sh; then
				tclconfig=$d
			fi
		done
	fi
fi
AC_MSG_CHECKING(for Tcl)
if test -r ${tclconfig}/tclConfig.sh; then
	. ${tclconfig}/tclConfig.sh
	if test -r ${tclconfig}/../generic/tcl.h; then
		TCL_INCLUDE=-I${tclconfig}/../generic
		TCL_LIB="$TCL_BUILD_LIB_SPEC $TCL_LIBS"
 	elif test -d ${TCL_PREFIX}/include/tcl${TCL_VERSION}; then
		TCL_INCLUDE=-I${TCL_PREFIX}/include/tcl${TCL_VERSION}
		TCL_LIB="$TCL_LIB_SPEC $TCL_LIBS"
	else
		TCL_INCLUDE=-I${TCL_PREFIX}/include
		TCL_LIB="$TCL_LIB_SPEC $TCL_LIBS"
	fi
	TCL_LIB=`echo $TCL_LIB|sed 's%-L/usr/lib%%g'`
        SHLIB_CFLAGS=$TCL_SHLIB_CFLAGS
        SHLIB_LD=$TCL_SHLIB_LD
        SHLIB_SUFFIX=$TCL_SHLIB_SUFFIX
        SHLIB_VERSION=$TCL_SHLIB_VERSION
        AC_MSG_RESULT($TCL_VERSION)
	AC_DEFINE(HAVE_TCL_H,1)

	# The Mac OSX -framework causes problems with Libtool
	# and dependancy libs.. so apply Tcl libs everywhere
	# bug #461
	case $host in
	*-*-darwin*)
		LIBS="$LIBS $TCL_LIB";
                ;;
	esac

else
        AC_MSG_RESULT(Not found)
	AC_DEFINE(HAVE_TCL_H,0)
fi
dnl
dnl ------ mkstemp
AC_CHECK_FUNCS(mkstemp)
dnl
dnl ------ GNU Readline
READLINE_SHARED_LIBADD=""
AC_CHECK_LIB(ncurses, tgetent, [READLINE_SHARED_LIBADD="-lncurses"],
        AC_CHECK_LIB(termcap, tgetent, [READLINE_SHARED_LIBADD="-ltermcap"])
)
READLINE_LIBS=""
AC_CHECK_LIB(readline, readline, [READLINE_LIBS="$READLINE_LIBS -lreadline $READLINE_SHARED_LIBADD"],,$READLINE_SHARED_LIBADD)
AC_CHECK_LIB(history, add_history, [READLINE_LIBS="$READLINE_LIBS -lhistory"])
if test "$ac_cv_lib_readline_readline" = "yes"; then
        AC_CHECK_HEADERS(readline/readline.h readline/history.h)
        xLIBS=$LIBS
        LIBS="$LIBS $READLINE_LIBS"
        AC_TRY_LINK([
        #include <stdio.h>
        #include <readline/readline.h>
        ],[
                rl_attempted_completion_over = 0;
        ],AC_DEFINE(HAVE_READLINE_COMPLETION_OVER))
        AC_TRY_LINK([
        #include <stdio.h>
        #include <readline/readline.h>
        ],[
                rl_completion_matches (0, 0);
        ],AC_DEFINE(HAVE_READLINE_RL_COMPLETION_MATCHES))
        LIBS=$xLIBS
fi
dnl
dnl ------ iconv
AC_ARG_WITH(iconv, [  --with-iconv[=DIR]        iconv library in DIR])
if test "$with_iconv" != "no"; then
	AC_MSG_CHECKING(for iconv)
	oldLIBS="$LIBS"
	oldCPPFLAGS="${CPPFLAGS}"
	if test "$with_iconv" != "yes" -a "$with_iconv" != ""; then
		LIBS="$LIBS -L${with_iconv}/lib"
		CPPFLAGS="${CPPFLAGS} -I${with_iconv}/include"
	fi
	AC_TRY_LINK([
		#include <iconv.h>
	],[
		iconv_t t = iconv_open("", "");
	],[
		AC_DEFINE(HAVE_ICONV_H)
		AC_MSG_RESULT(yes)
	],[
		LIBS="$LIBS -liconv"
		AC_TRY_LINK([
			#include <iconv.h>
		],[
			iconv_t t = iconv_open("", "");
		],[
			AC_DEFINE(HAVE_ICONV_H)
			AC_MSG_RESULT(yes)
		],[
			LIBS="$oldLIBS"
			CPPFLAGS="$oldCPPFLAGS"
			AC_MSG_RESULT(no)
		])
	])
fi
dnl
dnl ------- BZIP2
AC_CHECK_LIB(bz2,bzCompressInit)
if test "$ac_cv_lib_bz2_bzCompressInit" = "yes"; then
	AC_CHECK_HEADERS(bzlib.h)
else
	AC_CHECK_LIB(bz2,BZ2_bzCompressInit)
	if test "$ac_cv_lib_bz2_BZ2_bzCompressInit" = "yes"; then
		AC_CHECK_HEADERS(bzlib.h)
	fi
fi
dnl
dnl ------ -lm
AC_CHECK_LIB(m,sqrt)
dnl ------ -ldl
AC_CHECK_LIB(dl,dlopen)
dnl
dnl ------ EXPAT
expat=yes
AC_SUBST(EXPAT_LIBS)
AC_ARG_WITH(expat,   [  --with-expat[=DIR]        EXPAT library in DIR],[expat=$withval])
if test "$expat" != "no"; then
	xLIBS="$LIBS";
	xCFLAGS="$CFLAGS";
	if test "$expat" != "yes"; then
                EXPAT_CFLAGS="-I$expat/include"
                EXPAT_LIBS="-L$expat/lib"
		CFLAGS="$EXPAT_CFLAGS $CFLAGS"
		LIBS="$EXPAT_LIBS $LIBS"
	fi
	AC_CHECK_LIB(expat,XML_ParserCreate,[EXPAT_LIBS="$EXPAT_LIBS -lexpat"])
	if test "$ac_cv_lib_expat_XML_ParserCreate" = "yes"; then
		AC_CHECK_HEADERS(expat.h)
	fi
	LIBS="$xLIBS"
	CFLAGS="$xCFLAGS"
fi
dnl
dnl ------- 64 bit files
AC_MSG_CHECKING(for LFS)
AC_TRY_RUN([#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
int main(int argc, char **argv)
{
	off_t o;
	char tmp_str[32];
	int fd;
	struct flock area;
	if (sizeof(off_t) != 8) 
		exit (1);
	o = 2000000000;
	sprintf (tmp_str, "%Ld", o+o+o);
	if (strcmp (tmp_str, "6000000000"))
		exit (1);
	fd = creat ("config.tmp", 0644);
	if (fd < 0)
		exit (1);
	area.l_type = F_WRLCK;
	area.l_whence = SEEK_SET;
	area.l_len = area.l_start = 0L;
	if (fcntl(fd, F_SETLKW, &area))
		exit (1);
	close (fd);
	unlink ("config.tmp");
	exit (0);
}
],bits=64,bits=32,bits=32)
if test "$bits" = "64"; then
	AC_DEFINE(_FILE_OFFSET_BITS,64)
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
dnl ------ Modules
AC_SUBST(SHARED_MODULE_LA)
SHARED_MODULE_LA=""
AC_SUBST(STATIC_MODULE_OBJ)
STATIC_MODULE_OBJ=""
AC_SUBST(STATIC_MODULE_LADD)
STATIC_MODULE_LADD=""
AC_DEFUN([ZEBRA_MODULE],[
	AC_ARG_ENABLE(mod-$1,[$3],[myen=$enableval],[myen=$2])
	AC_MSG_CHECKING([for module $1])
	if test "$myen" = "yes"; then
	   myen="shared" 
	fi
	if test "$enable_shared" != "yes"; then
	   if test "$myen" = "shared"; then
	      myen="static"
	   fi
        fi
	m=`echo $1|tr .- __`
	if test "$myen" = "shared"; then
	   AC_MSG_RESULT([shared])
	   SHARED_MODULE_LA="${SHARED_MODULE_LA} mod-$1.la"
	elif test "$myen" = "static" -o "$myen" = "yes"; then
	   AC_MSG_RESULT([static])
	   STATIC_MODULE_OBJ="${STATIC_MODULE_OBJ} \$(mod_${m}_la_OBJECTS)"
	   STATIC_MODULE_LADD="${STATIC_MODULE_LADD} \$(mod_${m}_la_LADD)"
	   modcpp=`echo $1|tr abcdefghijklmnopqrstuvwxyz- ABCDEFGHIJKLMNOPQRSTUVWXYZ_`
	   AC_DEFINE_UNQUOTED([IDZEBRA_STATIC_$modcpp])
	elif test "$myen" = "no"; then
	   AC_MSG_RESULT([disabled])
	else
	   AC_MSG_RESULT([$myen])
	   AC_MSG_ERROR([invalid --enable-mod-$1 value. Use on,off,static or shared])
	fi
])

ZEBRA_MODULE(text,static,    [  --enable-mod-text       Text filter])
ZEBRA_MODULE(grs-sgml,static,[  --enable-mod-grs-sgml   Simple SGML/XML filter])
ZEBRA_MODULE(grs-regx,shared,[  --enable-mod-grs-regx   REGX/TCL filter])
ZEBRA_MODULE(grs-marc,shared,[  --enable-mod-grs-marc   MARC filter])
ZEBRA_MODULE(grs-csv,shared, [  --enable-mod-grs-csv    CSV filter])
ZEBRA_MODULE(grs-danbib,shared,[  --enable-mod-grs-danbib DanBib filter (DBC)])
ZEBRA_MODULE(safari,shared,  [  --enable-mod-safari Safari filter (DBC)])
if test "$ac_cv_header_expat_h" = "yes"; then
   def="shared"
else
   def="no"
fi
ZEBRA_MODULE(grs-xml,[$def], [  --enable-mod-grs-xml    XML filter (Expat based)])
if test "$XSLT_VER"; then
   def="shared"
else
   def="no"
fi
ZEBRA_MODULE(xslt,[$def],    [  --enable-mod-xslt       XSLT filter])
ZEBRA_MODULE(alvis,[$def],   [  --enable-mod-alvis      ALVIS filter])
dnl ------ ANSI C Header files
AC_STDC_HEADERS
if test "$ac_cv_header_stdc" = "no"; then
	AC_MSG_WARN(Your system doesn't seem to support ANSI C)
fi
AC_SUBST(IDZEBRA_SRC_ROOT)
AC_SUBST(IDZEBRA_BUILD_ROOT)
IDZEBRA_SRC_ROOT=`cd ${srcdir}; pwd`
IDZEBRA_BUILD_ROOT=`pwd`
dnl ------ Create Makefiles
AC_OUTPUT([
  Makefile
  util/Makefile
  bfile/Makefile
  dfa/Makefile
  dict/Makefile
  isamb/Makefile
  isams/Makefile
  isamc/Makefile
  rset/Makefile
  data1/Makefile
  recctrl/Makefile
  index/Makefile
  include/Makefile include/idzebra/Makefile
  tab/Makefile
  doc/Makefile
  doc/zebra.xml
  doc/common/Makefile
  doc/common/html.dsl
  doc/common/html.xsl
  doc/common/print.dsl
  doc/common/tkl.xsl
  test/Makefile test/gils/Makefile test/usmarc/Makefile test/api/Makefile
  test/xslt/Makefile
  test/xpath/Makefile
  test/rusmarc/Makefile test/cddb/Makefile test/malxml/Makefile 
  test/mbox/Makefile
  test/config/Makefile test/sort2/Makefile
  test/xelm/Makefile
  test/dmoz/Makefile test/sort/Makefile test/zsh/Makefile
  test/marcxml/Makefile test/charmap/Makefile test/codec/Makefile
  test/espec/Makefile
  test/filters/Makefile
  examples/Makefile examples/gils/Makefile examples/zthes/Makefile
  examples/marc21/Makefile examples/alvis-oai/Makefile
  idzebra.spec
  idzebra-config
  Doxyfile
],[sed s%idzebra_echo_source=yes%idzebra_echo_source=no%g < idzebra-config > util/idzebra-config && chmod +x idzebra-config util/idzebra-config])


echo \ '
Build and install binaries with the usual
  make
  make check
  make install

Build distribution tarball with
  make dist

Verify distribution tarball with
  make distcheck

Or just build the Debian packages without configuring
  dpkg-buildpackage -rfakeroot
'

if test  -z "$YAZLIB"; then
	echo "YAZ was not found. Use --with-yaz=DIR to specify location."
	test -f /etc/debian_version && echo "Debian package libyaz-dev is required."
fi

echo \
"------------------------------------------------------------------------
Configuration:

  Source code location:       ${srcdir}
  C Preprocessor flags:       ${CPPFLAGS}
  C Compiler:                 ${CC}
  C Compiler flags:           ${CFLAGS}
  Linker flags:               ${LDFLAGS}
  Linked libs:                ${LIBS}
  Host System Type:           ${host}
  Install path:               ${prefix}
  Automake:                   ${AUTOMAKE}

------------------------------------------------------------------------"