# Zebra makefile for MS NMAKE
# $Id: makefile,v 1.65 2007-08-22 12:36:22 adam Exp $
 
###########################################################
############### Parameters 
###########################################################

DEBUG=0   # 0 for release, 1 for debug

# EXPAT is optional. It's required for grs.xml-filters.
HAVE_EXPAT=1
EXPAT_DIR=c:\Program files\Expat 2.0.1

# iconv is optional
HAVE_ICONV=1
ICONV_DIR=c:\iconv-1.9.2.win32

# libxslt. For alvis
HAVE_LIBXSLT=1
LIBXSLT_DIR=c:\libxslt-1.1.19.win32

# libxml2. Used by libxslt
HAVE_LIBXML2=1
LIBXML2_DIR=c:\libxml2-2.6.28.win32

# zlib compression.  Used by libxml2
ZLIB_DIR = c:\zlib-1.2.3.win32

# get WIN32 binaries libxml2 & iconv & zlib from here:
#  http://www.zlatkovic.com/libxml.en.html

default: all

all: dirs expat iconv libxml2 libxslt yaz dll zserver zebraidx tstflock

# Directories
# The current directory is supposed to be something like
# ..../Zebra/Win, everything is relative to that
ROOTDIR=..   # The home of zebra

# YAZ include files, libraries, etc.
YAZDIR=$(ROOTDIR)\..\yaz  # or \program files\yaz

YAZINCL=$(YAZDIR)\include
YAZLIBS=$(YAZLIB)
YAZBINDIR=$(YAZDIR)\bin
!if $(DEBUG)
YAZLIB="$(YAZDIR)\lib\yaz3d.lib"
YAZ_DLL_SOURCE="$(YAZBINDIR)\yaz3d.dll"
YAZ_DLL_TARGET="$(BINDIR)\yaz3d.dll"
!else
YAZLIB="$(YAZDIR)\lib\yaz3.lib"
YAZ_DLL_SOURCE="$(YAZBINDIR)\yaz3.dll"
YAZ_DLL_TARGET="$(BINDIR)\yaz3.dll"
!endif

# BZIP2 settings. Uncomment and specify if you wish to use LIBBZIP2.
# (C) 1996-1999 Julian Seward <jseward@acm.org> http://www.bzip2.org

#BZIP2INCLUDE=/I"$(ROOTDIR)\..\bzip2-0.9.5d"
#BZIP2LIB=$(ROOTDIR)\..\bzip2-0.9.5d\libbz2.lib
#BZIP2DEF=/D"HAVE_BZLIB_H=1"

# ZEBRA Include files, libraries, programs, etc.
INCLDIR=$(ROOTDIR)\include  # our includes
LIBDIR=$(ROOTDIR)\lib       # We produce .lib, .exp etc there
BINDIR=$(ROOTDIR)\bin       # We produce exes and dlls there
WINDIR=$(ROOTDIR)\win       # all these Win make things
!if $(DEBUG)
OBJDIR=$(WINDIR)\dobj       # where we store intermediate files
!else
OBJDIR=$(WINDIR)\obj        # where we store intermediate files
!endif
UNIXDIR=$(ROOTDIR)\unix     # corresponding unix things
SRCDIR=$(ROOTDIR)           # for the case we move them under src

INDEXDIR=$(SRCDIR)\INDEX
BFILEDIR=$(SRCDIR)\BFILE
DFADIR=$(SRCDIR)\DFA
DICTDIR=$(SRCDIR)\DICT
ISAMSDIR=$(SRCDIR)\ISAMS
ISAMCDIR=$(SRCDIR)\ISAMC
ISAMBDIR=$(SRCDIR)\ISAMB
RSETDIR=$(SRCDIR)\RSET
UTILDIR=$(SRCDIR)\UTIL
DATA1DIR=$(SRCDIR)\DATA1

# Force temp files in a local temp, easier to clean
# when nmake crashes and leaves a lot of rubbish behind
TMPDIR=$(ROOTDIR)\win\tmp
TMP=$(TMPDIR)
TEMP=$(TMPDIR)

# NSIS
NSIS="c:\program files\nsis\makensis.exe"

dist:
	nmake DEBUG=1 clean
	nmake DEBUG=0 clean
	nmake DEBUG=0
	$(NSIS) zebra.nsi

nsis:
	$(NSIS) zebra.nsi

# Targets - what to make

!if $(DEBUG)
DLL=$(BINDIR)\idzebrad.dll
IMPLIB=$(LIBDIR)\idzebrad.lib
!else
DLL=$(BINDIR)\idzebra.dll
IMPLIB=$(LIBDIR)\idzebra.lib
!endif

ZEBRA_RES=$(OBJDIR)\idzebra.res

ZSERVER=$(BINDIR)\zebrasrv.exe
ZEBRAIDX=$(BINDIR)\zebraidx.exe
TSTFLOCK=$(BINDIR)\tstflock.exe

# shortcut names defined here
zebraidx: $(ZEBRAIDX)
zserver: $(ZSERVER)
tstflock: $(TSTFLOCK)
dll: $(DLL) 

# External libs and modules (enabled or disabled)

!if $(HAVE_EXPAT)
EXPAT_DEF= /D HAVE_EXPAT_H=1 /I"$(EXPAT_DIR)\source\lib"
EXPAT_LIB= "$(EXPAT_DIR)\bin\libexpat.lib"
EXPAT_DLL_SOURCE= "$(EXPAT_DIR)\bin\libexpat.dll"
EXPAT_DLL_TARGET= "$(BINDIR)\libexpat.dll"
expat: $(EXPAT_DLL_TARGET)

$(EXPAT_DLL_TARGET) : $(EXPAT_DLL_SOURCE)
	copy $(EXPAT_DLL_SOURCE) $(EXPAT_DLL_TARGET)
!else
EXPAT_DEF= /D HAVE_EXPAT_H=0
EXPAT_LIB=
EXPAT_DLL_SOURCE=
EXPAT_DLL_TARGET=
expat:
!endif

!if $(HAVE_ICONV)
ICONV_DEF= /D HAVE_ICONV_H=1 /I"$(ICONV_DIR)\include"
ICONV_LIB= $(ICONV_DIR)\lib\iconv.lib
iconv: $(BINDIR)\iconv.dll

$(BINDIR)\iconv.dll:
	copy "$(ICONV_DIR)\bin\iconv.dll" $(BINDIR)
!else
ICONV_DEF= /D HAVE_ICONV_H=0
ICONV_LIB=
iconv:
!endif

!if $(HAVE_LIBXML2)
LIBXML2_LIB="$(LIBXML2_DIR)\lib\libxml2.lib"
LIBXML2_DEF=/DYAZ_HAVE_XML2=1 /D HAVE_XML2=1 /I"$(LIBXML2_DIR)\include"
libxml2: $(BINDIR)\libxml2.dll $(BINDIR)\zlib1.dll

$(BINDIR)\libxml2.dll:
  copy "$(LIBXML2_DIR)\bin\libxml2.dll" $(BINDIR)

$(BINDIR)\zlib1.dll:
  copy "$(ZLIB_DIR)\bin\zlib1.dll" $(BINDIR)

!else
LIBXML2_LIB=
LIBXML2_DEF=/D HAVE_XML2=0
libxml2: 

!endif

!if $(HAVE_LIBXSLT)
LIBXSLT_LIB="$(LIBXSLT_DIR)\lib\libxslt.lib"
LIBXSLT_DEF=/D HAVE_XSLT=1 /I"$(LIBXSLT_DIR)\include"
libxslt: $(BINDIR)\libxslt.dll 

$(BINDIR)\libxslt.dll:
  copy "$(LIBXSLT_DIR)\bin\libxslt.dll" $(BINDIR)

!else
LIBXSLT_LIB=
LIBXSLT_DEF=/D HAVE_XSLT=0
libxslt: 

!endif

!if $(HAVE_LIBXSLT)
MOD_ALVIS_OBJS= $(OBJDIR)\alvis.obj
MOD_ALVIS_CFLAGS=/DIDZEBRA_STATIC_ALVIS=1

MOD_DOM_OBJS= $(OBJDIR)\mod_dom.obj
MOD_DOM_CFLAGS=/DIDZEBRA_STATIC_DOM=1
!else
MOD_ALVIS_OBJS=
MOD_ALVIS_CFLAGS=

MOD_DOM_OBJS=
MOD_DOM_CFLAGS=
!endif

### C and CPP compiler  (the same thing)
# Note: $(CPP) has already been defined in the environment
# (if you set things up right!)

COMMON_C_OPTIONS=          \
  /nologo /W3 /EHsc /FD /c \
  /D "WIN32"               \
  /D"_CONSOLE" /D"_MBCS"   \
  /DYAZ_USE_NEW_LOG=1      \
  /D"_CRT_SECURE_NO_DEPRECATE" \
  /D"_CRT_NONSTDC_NO_DEPRECATE" \
  $(ICONV_DEF)             \
  $(EXPAT_DEF)             \
  $(LIBXML2_DEF)           \
  $(LIBXSLT_DEF)           \
  /FR"$(OBJDIR)\\"         \
  /Fo"$(OBJDIR)\\"         \
  /Fd"$(OBJDIR)\\"         \
  /DDEFAULT_PROFILE_PATH=0 \
  /DDEFAULT_MODULE_PATH=0  \
  /DIDZEBRA_STATIC_GRS_SGML=1 \
  /DIDZEBRA_STATIC_TEXT=1  \
  /DIDZEBRA_STATIC_GRS_XML=1 \
  /DIDZEBRA_STATIC_GRS_REGX=1 \
  /DIDZEBRA_STATIC_GRS_MARC=1 \
  /DIDZEBRA_STATIC_GRS_DANBIB=1 \
  $(MOD_ALVIS_CFLAGS) \
  $(MOD_DOM_CFLAGS) \
  /DIDZEBRA_STATIC_SAFARI=1 \
  $(BZIP2DEF)

COMMON_C_INCLUDES= \
  /I"$(SRCDIR)\include" \
  /I"$(YAZDIR)\include" \
  $(BZIP2INCLUDE)

DEBUG_C_OPTIONS=  \
  /D "_DEBUG"      \
  /MDd  /Od /YX /Zi /Gm

RELEASE_C_OPTIONS=  \
  /D "NDEBUG"        \
  /MD /O2

# /W3  = warning level
# /GX  = Enable exception handling
# /FD  = Generate file dependencies (what ever they are)
# /c   = compile without linking
# /FR  = Generate browse info (.sbr file that gets combined into .bsc)
# /Fo  = object file name (or at least path)
# /Fd  = debug database name (or path)
# /MD  = Runtime library: Multithread DLL
# /MDd = Runtime library: Multithread DLL (debug)
# /Od  = Disable optimising (debug)
# /O2  = Optimize for speed
# /YX  = Automatic use of precomipled headers
# /Gm  = Minimal rebuild (some cpp class stuff)
# /Zi  = Program database for debuggers
# /ZI  = Pgm database with special "edit&continue" stuff - not available in C5

### Linker options
LINK=link.exe

LINK_LIBS= kernel32.lib user32.lib   gdi32.lib   winspool.lib \
           comdlg32.lib advapi32.lib shell32.lib ole32.lib    \
           oleaut32.lib uuid.lib     odbc32.lib  odbccp32.lib \
           wsock32.lib  advapi32.lib \
	   $(ICONV_LIB) $(EXPAT_LIB) $(LIBXML2_LIB) $(LIBXSLT_LIB)

COMMON_LNK_OPTIONS= /nologo /machine:i386 /incremental:no

DEBUG_LNK_OPTIONS= /debug 

RELEASE_LNK_OPTIONS= 

ZEBRALIB_LINK_OPTIONS= -lib 

DLL_LINK_OPTIONS= /dll  

CLIENT_LINK_OPTIONS= /subsystem:console  

# Final opt variables
!if $(DEBUG)
COPT=   $(COMMON_C_OPTIONS)   $(DEBUG_C_OPTIONS)     $(COMMON_C_INCLUDES)
MTLOPT= $(COMMON_MTL_OPTIONS) $(DEBUG_MTL_OPTIONS)
RCOPT=  $(COMMON_RC_OPTIONS)  $(DEBUG_RC_OPTIONS)
LNKOPT= $(COMMON_LNK_OPTIONS) $(DEBUG_LNK_OPTIONS)   $(LNK_LIBS)

!else
COPT=   $(COMMON_C_OPTIONS)   $(RELEASE_C_OPTIONS)   $(COMMON_C_INCLUDES) 
MTLOPT= $(COMMON_MTL_OPTIONS) $(RELEASE_MTL_OPTIONS)
RCOPT=  $(COMMON_RC_OPTIONS)  $(RELEASE_RC_OPTIONS)
LNKOPT= $(COMMON_LNK_OPTIONS) $(RELEASE_LNK_OPTIONS) $(LNK_LIBS)
!endif

LINK_PROGRAM= $(LINK) \
		$(LNKOPT) \
		$(CLIENT_LINK_OPTIONS) \
		$(IMPLIB) \
		$(YAZLIBS) \
		$(BZIP2LIB) \
		$(LINK_LIBS)

# Source and object modules
# Note: Ordinary source files are not specified here at 
# all, make finds them in suitable dirs. The object modules
# need to be specified, though

ZSERVER_OBJS= \
	$(OBJDIR)\zebrasrv.obj 

ZEBRAIDX_OBJS= \
	$(OBJDIR)\zebraidx.obj 
TSTFLOCK_OBJS= \
	$(OBJDIR)\tstflock.obj

ZEBRALIB_OBJS= \
        $(MOD_ALVIS_OBJS) \
        $(MOD_DOM_OBJS) \
	$(OBJDIR)\atoi_zn.obj \
	$(OBJDIR)\attribute.obj \
	$(OBJDIR)\attrfind.obj \
	$(OBJDIR)\bfile.obj \
	$(OBJDIR)\bset.obj \
	$(OBJDIR)\cfile.obj \
	$(OBJDIR)\check_res.obj \
	$(OBJDIR)\charmap.obj \
	$(OBJDIR)\close.obj \
	$(OBJDIR)\commit.obj \
	$(OBJDIR)\compact.obj \
	$(OBJDIR)\d1_absyn.obj \
	$(OBJDIR)\d1_attset.obj \
	$(OBJDIR)\d1_doespec.obj \
	$(OBJDIR)\d1_espec.obj \
	$(OBJDIR)\d1_expout.obj \
	$(OBJDIR)\d1_grs.obj \
	$(OBJDIR)\d1_handle.obj \
	$(OBJDIR)\d1_if.obj \
	$(OBJDIR)\d1_map.obj \
	$(OBJDIR)\d1_marc.obj \
	$(OBJDIR)\d1_prtree.obj \
	$(OBJDIR)\d1_read.obj \
	$(OBJDIR)\d1_soif.obj \
	$(OBJDIR)\d1_sumout.obj \
	$(OBJDIR)\d1_sutrs.obj \
	$(OBJDIR)\d1_tagset.obj \
	$(OBJDIR)\d1_utils.obj \
	$(OBJDIR)\d1_varset.obj \
	$(OBJDIR)\d1_write.obj \
	$(OBJDIR)\dclose.obj \
	$(OBJDIR)\dcompact.obj \
	$(OBJDIR)\delete.obj \
	$(OBJDIR)\dfa.obj \
	$(OBJDIR)\dir.obj \
	$(OBJDIR)\dirent.obj \
	$(OBJDIR)\dirs.obj \
	$(OBJDIR)\dopen.obj \
	$(OBJDIR)\drdwr.obj \
	$(OBJDIR)\exit.obj \
	$(OBJDIR)\extract.obj \
	$(OBJDIR)\flock.obj \
	$(OBJDIR)\imalloc.obj \
	$(OBJDIR)\inline.obj \
	$(OBJDIR)\insert.obj \
	$(OBJDIR)\invstat.obj \
	$(OBJDIR)\isamb.obj \
	$(OBJDIR)\isamc.obj \
	$(OBJDIR)\isams.obj \
	$(OBJDIR)\isam_methods.obj \
	$(OBJDIR)\it_key.obj \
	$(OBJDIR)\kcontrol.obj \
	$(OBJDIR)\key_block.obj \
	$(OBJDIR)\kinput.obj \
	$(OBJDIR)\limit.obj \
	$(OBJDIR)\lookgrep.obj \
	$(OBJDIR)\lookup.obj \
	$(OBJDIR)\lookupec.obj \
	$(OBJDIR)\marcomp.obj \
	$(OBJDIR)\marcread.obj \
	$(OBJDIR)\merge.obj \
	$(OBJDIR)\mfile.obj \
	$(OBJDIR)\open.obj \
	$(OBJDIR)\orddict.obj \
	$(OBJDIR)\passwddb.obj \
	$(OBJDIR)\rank1.obj \
	$(OBJDIR)\ranksimilarity.obj \
	$(OBJDIR)\rankstatic.obj \
	$(OBJDIR)\recctrl.obj \
	$(OBJDIR)\recgrs.obj \
	$(OBJDIR)\recindex.obj \
	$(OBJDIR)\reckeys.obj \
	$(OBJDIR)\recstat.obj \
	$(OBJDIR)\rectext.obj \
	$(OBJDIR)\regxread.obj \
	$(OBJDIR)\res.obj \
	$(OBJDIR)\retrieve.obj \
	$(OBJDIR)\rpnscan.obj \
	$(OBJDIR)\rpnsearch.obj \
	$(OBJDIR)\rsbetween.obj \
	$(OBJDIR)\rsbool.obj \
	$(OBJDIR)\rset.obj \
	$(OBJDIR)\rsisamb.obj \
	$(OBJDIR)\rsisamc.obj \
	$(OBJDIR)\rsisams.obj \
	$(OBJDIR)\rsmultiandor.obj \
	$(OBJDIR)\rsnull.obj \
	$(OBJDIR)\rsprox.obj \
	$(OBJDIR)\rstemp.obj \
	$(OBJDIR)\safari.obj \
	$(OBJDIR)\scan.obj \
	$(OBJDIR)\set.obj \
	$(OBJDIR)\sgmlread.obj \
	$(OBJDIR)\snippet.obj \
	$(OBJDIR)\sortidx.obj \
	$(OBJDIR)\states.obj \
	$(OBJDIR)\stream.obj \
	$(OBJDIR)\su_codec.obj \
	$(OBJDIR)\symtab.obj \
	$(OBJDIR)\trunc.obj \
	$(OBJDIR)\untrans.obj \
	$(OBJDIR)\update_path.obj \
	$(OBJDIR)\update_file.obj \
	$(OBJDIR)\xmlread.obj \
	$(OBJDIR)\xpath.obj \
	$(OBJDIR)\zaptterm.obj \
	$(OBJDIR)\zebra-lock.obj \
	$(OBJDIR)\zebraapi.obj \
	$(OBJDIR)\zebramap.obj \
	$(OBJDIR)\zinfo.obj \
	$(OBJDIR)\zint.obj \
	$(OBJDIR)\zsets.obj

# Compiling 
# Note: This defines where to look for the necessary
# source files. Funny way of doing it, but it works.

{$(INDEXDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(BFILEDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(DFADIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(DICTDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(ISAMSDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(ISAMCDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(ISAMBDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(RSETDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(UTILDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(DATA1DIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

# Linking

$(ZSERVER) : "$(BINDIR)" $(ZSERVER_OBJS) $(IMPLIB)
	$(LINK_PROGRAM) \
		$(ZSERVER_OBJS) \
		/out:$(ZSERVER)

$(ZEBRAIDX) : "$(BINDIR)" $(ZEBRAIDX_OBJS) $(IMPLIB)
	$(LINK_PROGRAM) \
		$(ZEBRAIDX_OBJS) \
		/out:$(ZEBRAIDX)

$(TSTFLOCK) : "$(BINDIR)" $(TSTFLOCK_OBJS) $(IMPLIB)
	$(LINK_PROGRAM) \
		$(TSTFLOCK_OBJS) \
		/out:$(TSTFLOCK) 

$(DLL) $(IMPLIB): "$(BINDIR)" $(ZEBRALIB_OBJS)
	$(LINK) \
		$(LNKOPT) \
		$(LINK_LIBS) \
		$(DLL_LINK_OPTIONS) \
		$(ZEBRALIB_OBJS) \
		/out:$(DLL) \
		$(YAZLIBS) \
		/implib:"$(IMPLIB)" \
		/map:"$(LIBDIR)\idzebra.map" \

# Other rules

clean:
	-del $(OBJDIR)\*.obj
	-del $(OBJDIR)\*.sbr
	-del $(BINDIR)\*.exe
	-del $(BINDIR)\*.dll
	-del $(TMPDIR)\*.
	-del $(LIBDIR)\*.lib

dirs: $(OBJDIR) $(WINDIR) $(LIBDIR) $(BINDIR) $(TMPDIR)

$(OBJDIR) $(WINDIR) $(LIBDIR) $(BINDIR) $(TMPDIR):
	if not exist "$@/$(NUL)" mkdir "$@"

yaz: $(YAZ_DLL_TARGET)

$(YAZ_DLL_TARGET) : $(YAZ_DLL_SOURCE)
#	copy "$(YAZBINDIR)\*.dll.manifest" $(BINDIR)
	copy "$(YAZBINDIR)\*.dll" $(BINDIR)


