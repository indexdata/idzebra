# Microsoft Developer Studio Project File - Name="zebra" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=zebra - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "zebra.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "zebra.mak" CFG="zebra - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zebra - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "zebra - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "zebra - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "include" /I "..\yaz\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "zebra - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "include" /I "..\yaz\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "zebra - Win32 Release"
# Name "zebra - Win32 Debug"
# Begin Source File

SOURCE=.\index\attribute.c
# End Source File
# Begin Source File

SOURCE=.\bfile\bfile.c
# End Source File
# Begin Source File

SOURCE=.\dfa\bset.c
# End Source File
# Begin Source File

SOURCE=.\bfile\cfile.c
# End Source File
# Begin Source File

SOURCE=.\util\charmap.c
# End Source File
# Begin Source File

SOURCE=.\dict\close.c
# End Source File
# Begin Source File

SOURCE=.\bfile\commit.c
# End Source File
# Begin Source File

SOURCE=.\index\compact.c
# End Source File
# Begin Source File

SOURCE=.\dict\dclose.c
# End Source File
# Begin Source File

SOURCE=.\dict\dcompact.c
# End Source File
# Begin Source File

SOURCE=.\dict\delete.c
# End Source File
# Begin Source File

SOURCE=.\dfa\dfa.c
# End Source File
# Begin Source File

SOURCE=.\util\dirent.c
# End Source File
# Begin Source File

SOURCE=.\dict\dopen.c
# End Source File
# Begin Source File

SOURCE=.\dict\drdwr.c
# End Source File
# Begin Source File

SOURCE=.\dfa\imalloc.c
# End Source File
# Begin Source File

SOURCE=.\dict\insert.c
# End Source File
# Begin Source File

SOURCE=.\index\invstat.c
# End Source File
# Begin Source File

SOURCE=.\isam\isam.c
# End Source File
# Begin Source File

SOURCE=.\isamc\isamc.c
# End Source File
# Begin Source File

SOURCE=.\isam\isutil.c
# End Source File
# Begin Source File

SOURCE=.\index\kcompare.c
# End Source File
# Begin Source File

SOURCE=.\index\lockidx.c
# End Source File
# Begin Source File

SOURCE=.\index\locksrv.c
# End Source File
# Begin Source File

SOURCE=.\index\lockutil.c
# End Source File
# Begin Source File

SOURCE=.\dict\lookgrep.c
# End Source File
# Begin Source File

SOURCE=.\dict\lookup.c
# End Source File
# Begin Source File

SOURCE=.\dict\lookupec.c
# End Source File
# Begin Source File

SOURCE=.\recctrl\marcread.c
# End Source File
# Begin Source File

SOURCE=.\isam\memory.c
# End Source File
# Begin Source File

SOURCE=.\isamc\merge.c
# End Source File
# Begin Source File

SOURCE=.\bfile\mfile.c
# End Source File
# Begin Source File

SOURCE=.\dict\open.c
# End Source File
# Begin Source File

SOURCE=.\util\passwddb.c
# End Source File
# Begin Source File

SOURCE=.\isam\physical.c
# End Source File
# Begin Source File

SOURCE=.\index\rank1.c
# End Source File
# Begin Source File

SOURCE=.\dfa\readfile.c
# End Source File
# Begin Source File

SOURCE=.\recctrl\recctrl.c
# End Source File
# Begin Source File

SOURCE=.\recctrl\recgrs.c
# End Source File
# Begin Source File

SOURCE=.\index\recindex.c
# End Source File
# Begin Source File

SOURCE=.\index\recstat.c
# End Source File
# Begin Source File

SOURCE=.\recctrl\rectext.c
# End Source File
# Begin Source File

SOURCE=.\recctrl\regxread.c
# End Source File
# Begin Source File

SOURCE=.\util\res.c
# End Source File
# Begin Source File

SOURCE=.\index\retrieve.c
# End Source File
# Begin Source File

SOURCE=.\isam\rootblk.c
# End Source File
# Begin Source File

SOURCE=.\rset\rsbool.c
# End Source File
# Begin Source File

SOURCE=.\rset\rset.c
# End Source File
# Begin Source File

SOURCE=.\rset\rsisam.c
# End Source File
# Begin Source File

SOURCE=.\rset\rsisamc.c
# End Source File
# Begin Source File

SOURCE=.\rset\rsm_or.c
# End Source File
# Begin Source File

SOURCE=.\rset\rsnull.c
# End Source File
# Begin Source File

SOURCE=.\rset\rstemp.c
# End Source File
# Begin Source File

SOURCE=.\dict\scan.c
# End Source File
# Begin Source File

SOURCE=.\dfa\set.c
# End Source File
# Begin Source File

SOURCE=.\recctrl\sgmlread.c
# End Source File
# Begin Source File

SOURCE=.\index\sortidx.c
# End Source File
# Begin Source File

SOURCE=.\dfa\states.c
# End Source File
# Begin Source File

SOURCE=.\index\symtab.c
# End Source File
# Begin Source File

SOURCE=.\index\trunc.c
# End Source File
# Begin Source File

SOURCE=.\index\zebraapi.c
# End Source File
# Begin Source File

SOURCE=.\util\zebramap.c
# End Source File
# Begin Source File

SOURCE=.\index\zinfo.c
# End Source File
# Begin Source File

SOURCE=.\index\zrpn.c
# End Source File
# Begin Source File

SOURCE=.\index\zsets.c
# End Source File
# End Target
# End Project
