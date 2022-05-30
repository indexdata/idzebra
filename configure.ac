dnl This file is part of the Zebra server.
dnl Copyright (C) Index Data. See LICENSE.zebra for details.
dnl
AC_PREREQ([2.69])
AC_INIT([idzebra],[m4_esyscmd([. ./IDMETA; printf $VERSION])],[info@indexdata.com])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([1.9])
PACKAGE_SUFFIX="-2.0"
AC_SUBST(PACKAGE_SUFFIX)
dnl https://www.gnu.org/software/libtool/manual/libtool.html#Libtool-versioning
ZEBRALIBS_VERSION_INFO=0:2:0
AC_SUBST([ZEBRALIBS_VERSION_INFO])
main_zebralib=index/libidzebra${PACKAGE_SUFFIX}.la
AC_SUBST(main_zebralib)
dnl ------ Substitutions
AC_SUBST([ZEBRA_CFLAGS])
AC_SUBST([AR_FLAGS],[cr])
dnl
dnl ------ Checking programs
AC_PROG_CC
AC_PROG_CPP
LT_INIT
PKG_PROG_PKG_CONFIG
dnl
dnl ------ headers
AC_CHECK_HEADERS([sys/resource.h sys/time.h sys/wait.h sys/utsname.h unistd.h])
dnl ------ crypt
AC_CHECK_LIB([crypt],[crypt])
if test "$ac_cv_lib_crypt_crypt" = "yes"; then
    AC_CHECK_HEADERS([crypt.h])
fi
dnl
dnl ------ YAZ
YAZ_INIT([server icu],[3.0.47])
if test "$YAZVERSION" = "NONE"; then
    AC_MSG_ERROR([YAZ development libraries required])
fi
YAZ_DOC

PKG_CHECK_MODULES([TCL], [tcl], [], [AC_DEFINE([HAVE_TCL_H], [0], [Define to 1 if we have TCL])])

dnl
dnl ------ various functions
AC_CHECK_FUNCS([mkstemp atoll])
dnkl
dnl ------ iconv
AC_ARG_WITH([iconv], [  --with-iconv[=DIR]        iconv library in DIR])
if test "$with_iconv" != "no"; then
    AC_MSG_CHECKING(for iconv)
    oldLIBS="$LIBS"
    oldCPPFLAGS="${CPPFLAGS}"
    if test "$with_iconv" != "yes" -a "$with_iconv" != ""; then
	LIBS="$LIBS -L${with_iconv}/lib"
	CPPFLAGS="${CPPFLAGS} -I${with_iconv}/include"
    fi
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
		#include <iconv.h>
	    ]], [[
	    iconv_t t = iconv_open("", "");
	    ]])],[
	    AC_DEFINE([HAVE_ICONV_H],[1],[Whether iconv.h is defined])
	    AC_MSG_RESULT(yes)
	    ],[
	    LIBS="$LIBS -liconv"
	    AC_LINK_IFELSE([AC_LANG_PROGRAM(
			    [[ #include <iconv.h> ]],[[ iconv_t t = iconv_open("", "");]])],[
		    AC_DEFINE([HAVE_ICONV_H],[1])
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
AC_CHECK_LIB([bz2],[bzCompressInit])
if test "$ac_cv_lib_bz2_bzCompressInit" = "yes"; then
    AC_CHECK_HEADERS([bzlib.h])
else
    AC_CHECK_LIB([bz2],[BZ2_bzCompressInit])
    if test "$ac_cv_lib_bz2_BZ2_bzCompressInit" = "yes"; then
	AC_CHECK_HEADERS([bzlib.h])
    fi
fi
dnl
dnl ------- zlib
AC_CHECK_LIB([z],[compress2])
if test "$ac_cv_lib_z_compress2" = "yes"; then
    AC_CHECK_HEADERS([zlib.h])
fi
dnl ------ -lm
AC_CHECK_LIB([m], [sqrt])
dnl ------ -ldl
AC_CHECK_LIB([dl], [dlopen])
dnl

PKG_CHECK_MODULES([EXPAT], [expat], [have_expat="yes"], [have_expat="no"])
if test "$have_expat" = "no"; then
    AC_CHECK_LIB([expat],[XML_ParserCreate],[EXPAT_LIBS="-lexpat"; have_expat=yes])
fi
if test "$have_expat" = "yes"; then
    AC_CHECK_HEADERS([expat.h])
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
AC_DEFINE_UNQUOTED([ZEBRA_ZINT],${ZINT_VALUE},[Whether zint is long long])
dnl ------ Modules
AC_SUBST([SHARED_MODULE_LA])
SHARED_MODULE_LA=""
AC_SUBST([STATIC_MODULE_OBJ])
STATIC_MODULE_OBJ=""
AC_SUBST([STATIC_MODULE_LADD])
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

AC_DEFINE([IDZEBRA_STATIC_GRS_SGML],[1],[Whether module grs.sgml is static])
AC_DEFINE([IDZEBRA_STATIC_TEXT],[0],[Whether module text is static])
ZEBRA_MODULE(text,shared,    [  --enable-mod-text       Text filter])
AC_DEFINE([IDZEBRA_STATIC_GRS_REGX],[0],[Whether module grs.regx is static])
ZEBRA_MODULE(grs-regx,shared,[  --enable-mod-grs-regx   REGX/TCL filter])
AC_DEFINE([IDZEBRA_STATIC_GRS_MARC],[0],[Whether module grs.marc is static])
ZEBRA_MODULE(grs-marc,shared,[  --enable-mod-grs-marc   MARC filter])
if test "$ac_cv_header_expat_h" = "yes"; then
    def="shared"
else
    def="disabled"
fi
AC_DEFINE([IDZEBRA_STATIC_GRS_XML],[0],[Whether module grs.xml is static])
ZEBRA_MODULE(grs-xml,[$def], [  --enable-mod-grs-xml    XML filter (Expat based)])
oldCPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $YAZINC"
AC_PREPROC_IFELSE(
   [AC_LANG_PROGRAM([[
#if YAZ_HAVE_XSLT
#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlIO.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#else
#error Libxslt not available
#endif
]],[[
#if LIBXML_VERSION < 20615
#error Libxml2 version < 2.6.15. xmlreader not reliable/present
#endif
]])],
   [def="shared"],
   [def="disabled"])
CPPFLAGS=$oldCPPFLAGS
AC_DEFINE([IDZEBRA_STATIC_DOM],[0],[Whether module dom is static])
ZEBRA_MODULE(dom,[$def],   [  --enable-mod-dom        XML/XSLT filter (Requires libxslt)])
AC_DEFINE([IDZEBRA_STATIC_ALVIS],[0],[Whether module alvis is static])
ZEBRA_MODULE(alvis,[$def], [  --enable-mod-alvis      ALVIS filter (Requires libxslt)])
AC_DEFINE([IDZEBRA_STATIC_SAFARI],[0],[Whether module safari is static])
ZEBRA_MODULE(safari,shared,[  --enable-mod-safari     Safari filter (DBC)])

AC_SUBST([IDZEBRA_SRC_ROOT])
AC_SUBST([IDZEBRA_BUILD_ROOT])
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
AC_CONFIG_FILES([
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
  doc/local0.ent
  doc/common/Makefile
  doc/common/print.dsl
  test/Makefile test/gils/Makefile test/usmarc/Makefile test/api/Makefile
  test/xslt/Makefile
  test/xpath/Makefile
  test/rusmarc/Makefile test/cddb/Makefile test/malxml/Makefile 
  test/mbox/Makefile
  test/config/Makefile
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
  idzebra-config-2.0
  zebra.pc
  Doxyfile
  win/version.nsi
  include/idzebra/version.h
])
AC_CONFIG_COMMANDS([default],[
	sed -e 's%echo_source=yes%echo_source=no%g; s%src_root=.*$%%g; s%build_root=.*%%g' \
		< idzebra-config-2.0 > util/idzebra-config-2.0 \
		&& chmod +x idzebra-config-2.0 util/idzebra-config-2.0
	diff doc/local.ent doc/local0.ent >/dev/null 2>/dev/null \
		|| cp doc/local0.ent doc/local.ent
],[])
AC_OUTPUT

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
