dnl This file is part of the Zebra server.
dnl   Copyright (C) 1994-2009 Index Data
dnl
AC_PREREQ(2.60)
AC_INIT([idzebra],[2.0.44],[zebra-help@indexdata.dk])
AC_CONFIG_SRCDIR(configure.ac)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([1.9])
PACKAGE_SUFFIX="-2.0"
AC_SUBST(PACKAGE_SUFFIX)
dnl http://www.gnu.org/software/libtool/manual.html#Versioning
ZEBRALIBS_VERSION_INFO=0:1:0
AC_SUBST(ZEBRALIBS_VERSION_INFO)
main_zebralib=index/libidzebra${PACKAGE_SUFFIX}.la
AC_SUBST(main_zebralib)
dnl ------ Substitutions
AC_SUBST(TCL_INCLUDE)
AC_SUBST(TCL_LIB)
AC_SUBST(READLINE_LIBS)
AC_SUBST(ZEBRA_CFLAGS)
dnl
dnl ------ Checking programs
AC_PROG_CC
AC_PROG_CPP
AM_PROG_LIBTOOL
AC_DEFINE_UNQUOTED(HOST_TRIPLET,"${host}")
dnl
dnl ------ headers
AC_CHECK_HEADERS([sys/resource.h sys/time.h sys/wait.h sys/utsname.h unistd.h])
dnl ------ crypt
AC_CHECK_LIB([crypt],[crypt])
if test "$ac_cv_lib_crypt_crypt" = "yes"; then
    AC_CHECK_HEADERS(crypt.h)
fi
dnl
dnl ------ YAZ
YAZ_INIT([server icu],[3.0.17])
if test "$YAZVERSION" = "NONE"; then
    AC_MSG_ERROR([YAZ development libraries required])
fi
YAZ_DOC
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

    old_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="${TCL_INCLUDE} $CPPFLAGS"
    AC_CHECK_HEADERS(tcl.h)
    CPPFLAGS=${old_CPPFLAGS}

    # The Mac OSX -framework causes problems with Libtool
    # and dependancy libs.. so apply Tcl libs everywhere bug #461
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
dnl ------ various functions
AC_CHECK_FUNCS(mkstemp atoll)
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
dnl ------- zlib
AC_CHECK_LIB([z],[compress2])
if test "$ac_cv_lib_z_compress2" = "yes"; then
    AC_CHECK_HEADERS([zlib.h])
fi
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
AC_SYS_LARGEFILE
AC_CHECK_TYPES([long long])
if test "${ac_cv_type_long_long}" = "yes"; then
    ZINT_VALUE=1
else
    ZINT_VALUE=0
fi 
ZEBRA_CFLAGS="-DZEBRA_ZINT=${ZINT_VALUE}"
AC_DEFINE_UNQUOTED(ZEBRA_ZINT,${ZINT_VALUE})
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
	if test "$myen" = "no" -o "$myen" = "disabled"; then
	    AC_MSG_RESULT([disabled])
	elif test "$2" = "disabled"; then
	    AC_MSG_RESULT([disabled])
	    AC_MSG_ERROR([Cannot enable mod-$1 because of missing libs (XML, etc)])
	elif test "$myen" = "shared"; then
	    AC_MSG_RESULT([shared])
	    SHARED_MODULE_LA="${SHARED_MODULE_LA} mod-$1.la"
	elif test "$myen" = "static"; then
	    AC_MSG_RESULT([static])
	    STATIC_MODULE_OBJ="${STATIC_MODULE_OBJ} \$(mod_${m}_la_OBJECTS)"
	    STATIC_MODULE_LADD="${STATIC_MODULE_LADD} \$(mod_${m}_la_LADD)"
	    modcpp=`echo $1|tr abcdefghijklmnopqrstuvwxyz- ABCDEFGHIJKLMNOPQRSTUVWXYZ_`
	    AC_DEFINE_UNQUOTED([IDZEBRA_STATIC_$modcpp])
	else
	    AC_MSG_RESULT([$myen])
	    AC_MSG_ERROR([invalid --enable-mod-$1 value. Use on,off,static or shared])
	fi
	])

AC_DEFINE(IDZEBRA_STATIC_GRS_SGML)
ZEBRA_MODULE(text,shared,    [  --enable-mod-text       Text filter])
ZEBRA_MODULE(grs-regx,shared,[  --enable-mod-grs-regx   REGX/TCL filter])
ZEBRA_MODULE(grs-marc,shared,[  --enable-mod-grs-marc   MARC filter])
if test "$ac_cv_header_expat_h" = "yes"; then
    def="shared"
else
    def="disabled"
fi
ZEBRA_MODULE(grs-xml,[$def], [  --enable-mod-grs-xml    XML filter (Expat based)])
oldCPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $YAZINC"
AC_PREPROC_IFELSE(
   [AC_LANG_PROGRAM([[
#if YAZ_HAVE_XML2
#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlIO.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#else
#error Libxml2 not available
#endif
]],[[
#if LIBXML_VERSION < 20615
#error Libxml2 version < 2.6.15. xmlreader not reliable/present
#endif
]])],
   [def="shared"],
   [def="disabled"])
CPPFLAGS=$oldCPPFLAGS
ZEBRA_MODULE(dom,[$def],   [  --enable-mod-dom        XML/XSLT filter (Requires libxslt)])
ZEBRA_MODULE(alvis,[$def], [  --enable-mod-alvis      ALVIS filter (Requires libxslt)])
ZEBRA_MODULE(safari,shared,[  --enable-mod-safari     Safari filter (DBC)])

dnl ------ ANSI C Header files
AC_STDC_HEADERS
if test "$ac_cv_header_stdc" = "no"; then
    AC_MSG_WARN([Your system does not seem to support ANSI C])
fi
AC_SUBST(IDZEBRA_SRC_ROOT)
AC_SUBST(IDZEBRA_BUILD_ROOT)
IDZEBRA_SRC_ROOT=`cd ${srcdir}; pwd`
IDZEBRA_BUILD_ROOT=`pwd`
dnl
dnl ------ versioning
dnl
WIN_FILEVERSION=`echo $PACKAGE_VERSION | $AWK 'BEGIN { FS = "."; } { m = $4; printf("%d,%d,%d,%d", $1, $2, $3 == "" ? "0" : $3, $4 == "" ? "1" : $4);}'`
AC_SUBST([WIN_FILEVERSION])
VERSION_HEX=`echo $PACKAGE_VERSION | $AWK 'BEGIN { FS = "."; } { printf("%x", ($1 * 256 + $2) * 256 + $3);}'`
AC_SUBST([VERSION_HEX])
if test -d ${srcdir}/.git; then
    VERSION_SHA1=`git show --pretty=format:%H|head -1`
else
    VERSION_SHA1=`head -1 ${srcdir}/ChangeLog|awk '{print $2}'`
fi
AC_SUBST([VERSION_SHA1])
dnl
dnl ------ Create Makefiles
dnl
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
  index/Makefile
  include/Makefile include/idzebra/Makefile
  tab/Makefile
  doc/Makefile
  doc/local.ent
  doc/common/Makefile
  doc/common/print.dsl
  test/Makefile test/gils/Makefile test/usmarc/Makefile test/api/Makefile
  test/xslt/Makefile
  test/xpath/Makefile
  test/rusmarc/Makefile test/cddb/Makefile test/malxml/Makefile 
  test/mbox/Makefile
  test/config/Makefile
  test/xelm/Makefile
  test/dmoz/Makefile test/zsh/Makefile
  test/marcxml/Makefile test/charmap/Makefile test/codec/Makefile
  test/espec/Makefile
  test/filters/Makefile
  examples/Makefile 
  examples/gils/Makefile 
  examples/marc21/Makefile 
  examples/marcxml/Makefile 
  examples/oai-pmh/Makefile
  examples/zthes/Makefile
  idzebra.spec
  idzebra-config-2.0
  Doxyfile
  win/version.nsi
  include/idzebra/version.h
],[sed s%echo_source=yes%echo_source=no%g < idzebra-config-2.0 > util/idzebra-config-2.0 && chmod +x idzebra-config-2.0 util/idzebra-config-2.0])

echo \
"------------------------------------------------------------------------

  ZEBRA Package:              ${PACKAGE}
  ZEBRA Version:              ${VERSION}
  Source code location:       ${srcdir}
  C Preprocessor:             ${CPP}
  C Preprocessor flags:       ${CPPFLAGS}
  C Compiler:                 ${CC}
  C Compiler flags:           ${CFLAGS}
  Linker flags:               ${LDFLAGS}
  Linked libs:                ${LIBS}
  Host System Type:           ${host}
  Install path:               ${prefix}
  Automake:                   ${AUTOMAKE}
  Archiver:                   ${AR}
  Ranlib:                     ${RANLIB}
  YAZ Version:                ${YAZVERSION}
  YAZ Include:                ${YAZINC}
  YAZ La Lib:                 ${YAZLALIB}
  YAZ Lib:                    ${YAZLIB}
  Bugreport:                  ${PACKAGE_BUGREPORT}

------------------------------------------------------------------------"
dnl Local Variables:
dnl mode:shell-script
dnl sh-indentation:2
dnl sh-basic-offset: 4
dnl End:
