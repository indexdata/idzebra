# Zebra makefile for MS NMAKE
# $Id: makefile,v 1.30 2004-08-25 09:18:49 adam Exp $
 
###########################################################
############### Parameters 
###########################################################

DEBUG=0   # 0 for release, 1 for debug

# EXPAT is optional. It's required for grs.xml-filters.
HAVE_EXPAT=1
EXPAT_DIR= c:\Expat-1.95.7

# iconv is optional
HAVE_ICONV=1
ICONV_DIR = c:\iconv-1.9.1.win32

default: all

all: dirs zebralib zserver zindex

###########################################################
############### Directories
###########################################################
# The current directory is supposed to be something like
# ..../Zebra/Win, everything is relative to that
ROOTDIR=..   # The home of zebra

# YAZ include files, libraries, etc.
YAZDIR=$(ROOTDIR)\..\YAZ
YAZINCL=$(YAZDIR)\include
YAZLIB=$(YAZDIR)\lib\yaz.lib
YAZLIBS=$(YAZLIB)
YAZBINDIR=$(YAZDIR)\bin
YAZ_DLL_SOURCE=$(YAZBINDIR)\yaz.dll
YAZ_DLL_TARGET=$(BINDIR)\yaz.dll

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
OBJDIR=$(WINDIR)\obj        # where we store intermediate files
UNIXDIR=$(ROOTDIR)\unix     # corresponding unix things
SRCDIR=$(ROOTDIR)           # for the case we move them under src

INDEXDIR=$(SRCDIR)\INDEX
BFILEDIR=$(SRCDIR)\BFILE
DFADIR=$(SRCDIR)\DFA
DICTDIR=$(SRCDIR)\DICT
ISAMSDIR=$(SRCDIR)\ISAMS
ISAMDIR=$(SRCDIR)\ISAM
ISAMCDIR=$(SRCDIR)\ISAMC
ISAMBDIR=$(SRCDIR)\ISAMB
RECCTRLDIR=$(SRCDIR)\RECCTRL
RSETDIR=$(SRCDIR)\RSET
UTILDIR=$(SRCDIR)\UTIL
DATA1DIR=$(SRCDIR)\DATA1
APITESTDIR=$(SRCDIR)\APITEST

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

###########################################################
############### Targets - what to make
###########################################################

ZEBRALIB=$(LIBDIR)\zebra.lib
ZSERVER=$(BINDIR)\zebrasrv.exe
ZINDEX=$(BINDIR)\zebraidx.exe
APITEST=$(BINDIR)\apitest.exe

# shortcut names defined here
zebralib: $(ZEBRALIB)
zindex: $(ZINDEX)
zserver: $(ZSERVER)
apitest: $(APITEST)

###########################################################
############### Compiler and linker options 
###########################################################

!if $(HAVE_EXPAT)
EXPAT_DEF= /D HAVE_EXPAT_H=1 /I"$(EXPAT_DIR)\source\lib"
EXPAT_LIB= "$(EXPAT_DIR)\libs\libexpat.lib"
EXPAT_DLL_SOURCE= "$(EXPAT_DIR)\libs\libexpat.dll"
EXPAT_DLL_TARGET="$(BINDIR)\libexpat.dll"
$(EXPAT_DLL_TARGET) : $(EXPAT_DLL_SOURCE)
	copy $(EXPAT_DLL_SOURCE) $(EXPAT_DLL_TARGET)
!else
EXPAT_DEF= /D HAVE_EXPAT_H=0
EXPAT_LIB=
EXPAT_DLL_SOURCE=
EXPAT_DLL_TARGET=
!endif

!if $(HAVE_ICONV)
ICONV_DEF= /D HAVE_ICONV_H=1 /I"$(ICONV_DIR)\include"
ICONV_LIB= $(ICONV_DIR)\lib\iconv.lib
!else
ICONV_DEF= /D HAVE_ICONV_H=0
ICONV_LIB=
!endif

### C and CPP compiler  (the same thing)
# Note: $(CPP) has already been defined in the environment
# (if you set things up right!)

COMMON_C_OPTIONS=          \
  /nologo /W3 /GX /FD /c   \
  /D "WIN32"               \
  /D"_CONSOLE" /D"_MBCS"   \
  $(ICONV_DEF)             \
  $(EXPAT_DEF)             \
  /FR"$(OBJDIR)\\"         \
  /Fo"$(OBJDIR)\\"         \
  /Fd"$(OBJDIR)\\"         \
  /DDEFAULT_PROFILE_PATH=0 \
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
           wsock32.lib  advapi32.lib $(ICONV_LIB) $(EXPAT_LIB)

COMMON_LNK_OPTIONS= /nologo /machine:i386 /incremental:no

DEBUG_LNK_OPTIONS= /debug 

RELEASE_LNK_OPTIONS= 

ZEBRALIB_LINK_OPTIONS= -lib 

ZSERVER_LINK_OPTIONS= /subsystem:console  
ZINDEX_LINK_OPTIONS= /subsystem:console  
APITEST_LINK_OPTIONS= /subsystem:console  

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

###########################################################
###############  Source and object modules
###########################################################

# Note: Ordinary source files are not specified here at 
# all, make finds them in suitable dirs. The object modules
# need to be specified, though

ZSERVER_OBJS= \
	$(OBJDIR)\zserver.obj 

ZEBRAIDX_OBJS= \
	$(OBJDIR)\main.obj 
APITEST_OBJS= \
	$(OBJDIR)\apitest.obj

ZEBRALIB_OBJS= \
	$(OBJDIR)\isamc.obj \
	$(OBJDIR)\merge.obj \
	$(OBJDIR)\zebra-lock.obj \
	$(OBJDIR)\attribute.obj \
	$(OBJDIR)\bfile.obj \
	$(OBJDIR)\bset.obj \
	$(OBJDIR)\cfile.obj \
	$(OBJDIR)\charmap.obj \
	$(OBJDIR)\atoi_zn.obj \
	$(OBJDIR)\close.obj \
	$(OBJDIR)\commit.obj \
	$(OBJDIR)\compact.obj \
	$(OBJDIR)\dclose.obj \
	$(OBJDIR)\dcompact.obj \
	$(OBJDIR)\delete.obj \
	$(OBJDIR)\dfa.obj \
	$(OBJDIR)\dirent.obj \
	$(OBJDIR)\dopen.obj \
	$(OBJDIR)\drdwr.obj \
	$(OBJDIR)\imalloc.obj \
	$(OBJDIR)\insert.obj \
	$(OBJDIR)\invstat.obj \
	$(OBJDIR)\kcompare.obj \
	$(OBJDIR)\lockutil.obj \
	$(OBJDIR)\lookgrep.obj \
	$(OBJDIR)\lookup.obj \
	$(OBJDIR)\lookupec.obj \
	$(OBJDIR)\marcread.obj \
	$(OBJDIR)\marcomp.obj \
	$(OBJDIR)\inline.obj \
	$(OBJDIR)\mfile.obj \
	$(OBJDIR)\open.obj \
	$(OBJDIR)\passwddb.obj \
	$(OBJDIR)\zvrank.obj \
	$(OBJDIR)\rank1.obj \
	$(OBJDIR)\livcode.obj \
	$(OBJDIR)\readfile.obj \
	$(OBJDIR)\recctrl.obj \
	$(OBJDIR)\recgrs.obj \
	$(OBJDIR)\recindex.obj \
	$(OBJDIR)\recstat.obj \
	$(OBJDIR)\rectext.obj \
	$(OBJDIR)\regxread.obj \
	$(OBJDIR)\xmlread.obj \
	$(OBJDIR)\danbibr.obj \
	$(OBJDIR)\res.obj \
	$(OBJDIR)\retrieve.obj \
	$(OBJDIR)\rsbool.obj \
	$(OBJDIR)\rset.obj \
	$(OBJDIR)\rsisamc.obj \
	$(OBJDIR)\rsmultior.obj \
	$(OBJDIR)\rsnull.obj \
	$(OBJDIR)\rstemp.obj \
	$(OBJDIR)\scan.obj \
	$(OBJDIR)\set.obj \
	$(OBJDIR)\sgmlread.obj \
	$(OBJDIR)\sortidx.obj \
	$(OBJDIR)\states.obj \
	$(OBJDIR)\symtab.obj \
	$(OBJDIR)\trunc.obj \
	$(OBJDIR)\zebraapi.obj \
	$(OBJDIR)\zebramap.obj \
	$(OBJDIR)\zinfo.obj \
	$(OBJDIR)\zrpn.obj \
	$(OBJDIR)\zsets.obj \
	$(OBJDIR)\isams.obj \
	$(OBJDIR)\isamb.obj \
	$(OBJDIR)\rsisamb.obj \
	$(OBJDIR)\rsbetween.obj \
	$(OBJDIR)\rsprox.obj \
	$(OBJDIR)\dir.obj \
	$(OBJDIR)\dirs.obj \
	$(OBJDIR)\extract.obj \
	$(OBJDIR)\kinput.obj \
	$(OBJDIR)\rsisams.obj \
	$(OBJDIR)\trav.obj \
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
	$(OBJDIR)\d1_varset.obj \
	$(OBJDIR)\d1_write.obj \
	$(OBJDIR)\xpath.obj

ALL_OBJS=\
  $(ZEBRALIB_OBJS) \
  $(ZEBRAIDX_OBJS) \
  $(APITEST_OBJS)  \
  $(ZSERVER_OBJS)

###########################################################
############### Compiling 
###########################################################

# Note: This defines where to look for the necessary
# source files. Funny way of doing it, but it works.

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $<

{$(SRCDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $<

# ZebraLib
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

{$(ISAMDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(ISAMCDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(ISAMBDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(RECCTRLDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(RSETDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(UTILDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(DATA1DIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

{$(APITESTDIR)}.c{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

###########################################################
############### Linking
###########################################################

$(ZSERVER) : "$(BINDIR)" $(ZSERVER_OBJS) $(ZEBRALIB) $(YAZLIBS)
	@echo Linking the server $(ZSERVER)
	$(LINK) @<<
		$(LNKOPT) 
		$(ZSERVER_LINK_OPTIONS)
		/out:$(ZSERVER) 
		$(YAZLIBS)
		$(ZEBRALIB)
		$(BZIP2LIB)
		$(LINK_LIBS) 
		$(ZSERVER_OBJS) 
<<

$(ZINDEX) : "$(BINDIR)" $(ZEBRAIDX_OBJS) $(ZEBRALIB) $(YAZLIBS)
	@echo Linking the indexer $(ZINDEX)
	$(LINK) @<<
		$(LNKOPT) 
		$(ZINDEX_LINK_OPTIONS)
		/out:$(ZINDEX) 
		$(YAZLIBS)
		$(ZEBRALIB)
		$(BZIP2LIB)
		$(LINK_LIBS) 
		$(ZEBRAIDX_OBJS) 
<<

$(APITEST) : "$(BINDIR)" $(APITEST_OBJS) $(ZEBRALIB) $(YAZLIBS)
	@echo Linking the test program $(APITEST)
	$(LINK) @<<
		$(LNKOPT) 
		$(APITEST_LINK_OPTIONS)
		/out:$(APITEST) 
		$(YAZLIBS)
		$(ZEBRALIB)
		$(BZIP2LIB)
		$(LINK_LIBS) 
		$(APITEST_OBJS)
<<

$(ZEBRALIB) : "$(BINDIR)" $(ZEBRALIB_OBJS) 
	@echo Linking library  $(ZEBRALIB)
	$(LINK) $(ZEBRALIB_LINK_OPTIONS) @<<
		/nologo
		$(ZEBRALIB_OBJS) 
		/out:$(ZEBRALIB) 
<<

###########################################################
############### Special operations
###########################################################

clean:
	-del $(OBJDIR)\*.obj
	-del $(OBJDIR)\*.sbr
	-del $(BINDIR)\*.exe
	-del $(TMPDIR)\*.
	-del $(LIBDIR)\*.lib

########### check directories and create if needed
dirs: $(OBJDIR) $(WINDIR) $(LIBDIR) $(BINDIR) $(TMPDIR) \
	$(EXPAT_DLL_TARGET) $(YAZ_DLL_TARGET)

$(OBJDIR) $(WINDIR) $(LIBDIR) $(BINDIR) $(TMPDIR):
 	if not exist "$@/$(NUL)" mkdir "$@"

$(YAZ_DLL_TARGET) : $(YAZ_DLL_SOURCE)
	copy $(YAZ_DLL_SOURCE) $(YAZ_DLL_TARGET)

###########################################################
############### Explicit dependencies
###########################################################

# $(ALL_OBJS): makefile

# force recompilation of everything, if makefile changed

###########################################################
