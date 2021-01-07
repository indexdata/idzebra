# Zebra makefile for MS NMAKE
 
###########################################################
############### Parameters 
###########################################################

DEBUG=0   # 0 for release, 1 for debug
BARCH = 32
USE_MANIFEST = 0

# TCL is optional - ONLY needed for source straight from Git
TCL=tclsh
#TCL="C:\Tcl\bin\tclsh85.exe"
HAVE_TCL=1

# iconv is optional
HAVE_ICONV=0
ICONV_DIR=c:\iconv-1.9.2.win32

# EXPAT is optional. It's required for grs.xml-filters.
HAVE_EXPAT=1
EXPAT_DIR=c:\Program files (x86)\Expat 2.2.10

# ICU support
HAVE_ICU=1
ICU_VER=68
ICU_BASE=c:\icu4c-$(ICU_VER)_2-Win$(BARCH)-MSVC2019
ICU_INCLUDE=$(ICU_BASE)\include

# libxslt. For mod_dom and mod_alvis
HAVE_LIBXSLT=1
LIBXSLT_DIR=c:\libxslt-1.1.28.win$(BARCH)

# libxml2. Used by libxslt
HAVE_LIBXML2=1
LIBXML2_DIR=c:\libxml2-2.9.2.win$(BARCH)

default: all
all: dirs generate expat iconv yaz dll zserver zebraidx tstflock

# Directories
# The current directory is supposed to be something like
# ..../Zebra/Win, everything is relative to that
ROOTDIR=..   # The home of zebra

# YAZ include files, libraries, etc.
YAZDIR=$(ROOTDIR)\..\yaz  # or \program files\yaz

YAZBINDIR=$(YAZDIR)\bin
!if $(DEBUG)
YAZLIB="$(YAZDIR)\lib\yaz5d.lib"
!if $(HAVE_ICU)
YAZLIB=$(YAZLIB) "$(YAZDIR)\lib\yaz_icu5d.lib"
!endif
YAZ_DLL_SOURCE="$(YAZBINDIR)\yaz5d.dll"
YAZ_DLL_TARGET="$(BINDIR)\yaz5d.dll"
!else
YAZLIB="$(YAZDIR)\lib\yaz5.lib"
!if $(HAVE_ICU)
YAZLIB=$(YAZLIB) "$(YAZDIR)\lib\yaz_icu5.lib"
!endif
YAZ_DLL_SOURCE="$(YAZBINDIR)\yaz5.dll"
YAZ_DLL_TARGET="$(BINDIR)\yaz5.dll"
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
M4DIR=$(SRCDIR)\m4

# Force temp files in a local temp, easier to clean
# when nmake crashes and leaves a lot of rubbish behind
TMPDIR=$(ROOTDIR)\win\tmp
TMP=$(TMPDIR)
TEMP=$(TMPDIR)

# NSIS
NSIS="c:\program files (x86)\nsis\makensis.exe"

dist: version.nsi
	nmake DEBUG=1 clean
	nmake DEBUG=0 clean
	nmake DEBUG=0
	nmake nsis

nsis: version.nsi
	$(NSIS) /DVSARCH=x86 zebra.nsi

version.nsi: $(ROOTDIR)/IDMETA
	$(TCL) $(M4DIR)/mk_version.tcl $(ROOTDIR)/IDMETA version.nsi

# Targets - what to make

!if $(DEBUG)
DLL=$(BINDIR)\idzebrad.dll
IMPLIB=$(LIBDIR)\idzebrad.lib
!else
DLL=$(BINDIR)\idzebra.dll
IMPLIB=$(LIBDIR)\idzebra.lib
!endif

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
LIBXML2_DEF=/DYAZ_HAVE_XML2=1 /D HAVE_XML2=1 /I"$(LIBXML2_DIR)\include\libxml2"
!else
LIBXML2_LIB=
LIBXML2_DEF=/D HAVE_XML2=0
!endif

!if $(HAVE_LIBXSLT)
LIBXSLT_LIB="$(LIBXSLT_DIR)\lib\libxslt.lib"
LIBXSLT_DEF=/D HAVE_XSLT=1 /I"$(LIBXSLT_DIR)\include"
!else
LIBXSLT_LIB=
LIBXSLT_DEF=/D HAVE_XSLT=0

!endif

!if $(HAVE_ICU)
ICU_DEF= /DYAZ_HAVE_ICU=1 /I"$(ICU_INCLUDE)"
!else
ICU_DEF= /DYAZ_HAVE_ICU=0
!endif

!if $(HAVE_LIBXSLT)
MOD_ALVIS_OBJS= $(OBJDIR)\mod_alvis.obj
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
  /DYAZ_USE_NEW_LOG=1      \
  /D"_CRT_SECURE_NO_DEPRECATE" \
  /D"_CRT_NONSTDC_NO_DEPRECATE" \
  /D "WIN32"               \
  $(ICONV_DEF)             \
  $(EXPAT_DEF)             \
  $(LIBXML2_DEF)           \
  $(LIBXSLT_DEF)           \
  $(ICU_DEF)               \
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
  /D "_DEBUG"     \
  /MDd /Od /Zi /Gm

RELEASE_C_OPTIONS= \
  /D "NDEBUG"      \
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

!if $(USE_MANIFEST)
MT=mt.exe /nologo
!else
MT=echo
!endif

# wsock32.lib  advapi32.lib \

LINK_LIBS= kernel32.lib advapi32.lib \
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
		$(YAZLIB) \
		$(BZIP2LIB) \
		$(LINK_LIBS)


# Generated sources
generate: $(INCLDIR)\idzebra\version.h

$(INCLDIR)\idzebra\version.h: $(ROOTDIR)/IDMETA 
	@cd $(M4DIR)
	$(TCL) mk_version.tcl $(ROOTDIR)/IDMETA $(INCLDIR)\idzebra\version.h

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
	$(OBJDIR)\mod_grs_marc.obj \
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
	$(OBJDIR)\records.obj \
	$(OBJDIR)\recindex.obj \
	$(OBJDIR)\reckeys.obj \
	$(OBJDIR)\mod_text.obj \
	$(OBJDIR)\mod_grs_regx.obj \
	$(OBJDIR)\res.obj \
	$(OBJDIR)\retrieve.obj \
	$(OBJDIR)\rpnscan.obj \
	$(OBJDIR)\rpnsearch.obj \
	$(OBJDIR)\rsbetween.obj \
	$(OBJDIR)\rsbool.obj \
	$(OBJDIR)\rset.obj \
	$(OBJDIR)\rset_isam.obj \
	$(OBJDIR)\rsisamb.obj \
	$(OBJDIR)\rsisamc.obj \
	$(OBJDIR)\rsisams.obj \
	$(OBJDIR)\rsmultiandor.obj \
	$(OBJDIR)\rsnull.obj \
	$(OBJDIR)\rsprox.obj \
	$(OBJDIR)\rstemp.obj \
	$(OBJDIR)\mod_safari.obj \
	$(OBJDIR)\scan.obj \
	$(OBJDIR)\set.obj \
	$(OBJDIR)\mod_grs_sgml.obj \
	$(OBJDIR)\snippet.obj \
	$(OBJDIR)\sortidx.obj \
	$(OBJDIR)\states.obj \
	$(OBJDIR)\stream.obj \
	$(OBJDIR)\strmap.obj \
	$(OBJDIR)\su_codec.obj \
	$(OBJDIR)\trunc.obj \
	$(OBJDIR)\untrans.obj \
	$(OBJDIR)\update_path.obj \
	$(OBJDIR)\update_file.obj \
	$(OBJDIR)\version.obj \
	$(OBJDIR)\mod_grs_xml.obj \
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
	$(LINK_PROGRAM) $(ZSERVER_OBJS)  /out:$@
	$(MT) -manifest $@.manifest -outputresource:$@;1

$(ZEBRAIDX) : "$(BINDIR)" $(ZEBRAIDX_OBJS) $(IMPLIB)
	$(LINK_PROGRAM) $(ZEBRAIDX_OBJS) /out:$@
	$(MT) -manifest $@.manifest -outputresource:$@;1

$(TSTFLOCK) : "$(BINDIR)" $(TSTFLOCK_OBJS) $(IMPLIB)
	$(LINK_PROGRAM) $(TSTFLOCK_OBJS) /out:$@
	$(MT) -manifest $@.manifest -outputresource:$@;1

# Resources
RES=$(OBJDIR)\zebra.res
RC=$(WINDIR)\zebra.rc

RSC=rc.exe
!if $(DEBUG)
RSOPT=/d_DEBUG
!else
RSOPT=/d_NDEBUG
!endif


$(RES): $(RC)
	$(RSC) $(RSOPT) /I"../include" /I"$(YAZDIR)\include" /fo"$(RES)" $(RC)

$(DLL) $(IMPLIB): "$(BINDIR)" $(ZEBRALIB_OBJS) $(RES)
	$(LINK) \
		$(LNKOPT) \
		$(LINK_LIBS) \
		$(DLL_LINK_OPTIONS) \
		$(ZEBRALIB_OBJS) \
		$(RES) \
		/out:$@ \
		$(YAZLIB) \
		/implib:"$(IMPLIB)" \
		/map:"$(LIBDIR)\idzebra.map" 
	$(MT) -manifest $@.manifest -outputresource:$@;2

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


