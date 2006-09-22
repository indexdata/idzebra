; $Id: zebra.nsi,v 1.35 2006-09-22 12:44:04 adam Exp $

!define VERSION "2.0.2"

; VS 2005
; !define VS_RUNTIME_DLL      "c:\Program Files\Microsoft Visual Studio 8\VC\redist\x86\Microsoft.VC80.CRT\msvcr80.dll"
; !define VS_RUNTIME_MANIFEST "c:\Program Files\Microsoft Visual Studio 8\VC\redist\x86\Microsoft.VC80.CRT\Microsoft.VC80.CRT.manifest"

; VS 2003
!define VS_RUNTIME_DLL "c:\Program files\Microsoft Visual Studio .NET 2003\SDK\v1.1\Bin\msvcr71.dll"

!include "MUI.nsh"

SetCompressor bzip2

Name "Zebra"
Caption "Index Data Zebra ${VERSION} Setup"
OutFile "idzebra_${VERSION}.exe"

LicenseText "You must read the following license before installing:"
LicenseData license.txt

ComponentText "This will install Zebra on your computer:"
InstType "Full (w/ Source)"
InstType "Lite (w/o Source)"

InstallDir "$PROGRAMFILES\Zebra"
InstallDirRegKey HKLM "SOFTWARE\Index Data\Zebra" ""

;--------------------------------
; Pages

  !insertmacro MUI_PAGE_LICENSE "license.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
; Page components
; Page directory
; Page instfiles

; UninstPage uninstConfirm
; UninstPage instfiles

;--------------------------------
;Languages
 
!insertmacro MUI_LANGUAGE "English"

;--------------------------------
Section "" ; (default section)
	SetOutPath "$INSTDIR"
	; add files / whatever that need to be installed here.
	WriteRegStr HKLM "SOFTWARE\Index Data\Zebra" "" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Zebra" "DisplayName" "Zebra ${VERSION} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Zebra" "UninstallString" '"$INSTDIR\uninst.exe"'
	; write out uninstaller
	WriteUninstaller "$INSTDIR\uninst.exe"
	SetOutPath "$SMPROGRAMS\Index Data\Zebra\"
 	CreateShortCut "$SMPROGRAMS\Index Data\Zebra\Zebra Program Directory.lnk" \
                 "$INSTDIR"
	WriteINIStr "$SMPROGRAMS\Index Data\Zebra\Zebra Home page.url" \
              "InternetShortcut" "URL" "http://www.indexdata.dk/zebra/"
	CreateShortCut "$SMPROGRAMS\Index Data\Zebra\Uninstall Zebra.lnk" \
		"$INSTDIR\uninst.exe"
	SetOutPath $INSTDIR
	File ..\LICENSE.zebra
	File ..\README
	SetOutPath $INSTDIR
	File /r ..\tab
SectionEnd ; end of default section

Section "Zebra Runtime"
	SectionIn 1 2
	SetOutPath $INSTDIR\bin
	File "${VS_RUNTIME_DLL}"
; File "${VS_RUNTIME_MANIFEST}"
	File ..\bin\*.exe
; File ..\bin\*.manifest
	File ..\bin\idzebra.dll
	File ..\bin\iconv.dll
	File ..\bin\zlib1.dll
	File ..\bin\libxml2.dll
	File ..\bin\libxslt.dll
	File ..\bin\yaz.dll
	File ..\bin\libexpat.dll
SectionEnd

Section "Zebra Development"
	SectionIn 1
	SetOutPath $INSTDIR\include
	File ..\include\*.h
	SetOutPath $INSTDIR\include\idzebra
	File ..\include\idzebra\*.h
	SetOutPath $INSTDIR\lib
	File ..\lib\*.lib
SectionEnd

Section "Zebra Documentation"
	SectionIn 1 2
	SetOutPath $INSTDIR\doc
	File /r ..\doc\*.html
	File /r ..\doc\*.png
	File /r ..\doc\*.xml
	File /r ..\doc\*.xsl
	SetOutPath "$SMPROGRAMS\Index Data\Zebra\"
	CreateShortCut "$SMPROGRAMS\Index Data\Zebra\HTML Documentation.lnk" \
                 "$INSTDIR\doc\index.html"
SectionEnd

Section "Zebra Examples"
	SectionIn 1 2

	SetOutPath $INSTDIR

	File /r ..\test
	File /r /x *.mf /x *.LCK ..\examples
SectionEnd

Section "Zebra Source"
	SectionIn 1
	SetOutPath $INSTDIR\util
	File ..\util\*.c
	SetOutPath $INSTDIR\dfa
	File ..\dfa\*.c
	File ..\dfa\*.h
	SetOutPath $INSTDIR\index
	File ..\index\*.c
	File ..\index\*.h
	SetOutPath $INSTDIR\isams
	File ..\isams\*.c
	SetOutPath $INSTDIR\isamc
	File ..\isamc\*.c
	File ..\isamc\*.h
	SetOutPath $INSTDIR\isamb
	File ..\isamb\*.c
	SetOutPath $INSTDIR\data1
	File ..\data1\*.c
	SetOutPath $INSTDIR\dict
	File ..\dict\*.c
	File ..\dict\*.h
	SetOutPath $INSTDIR\bfile
	File ..\bfile\*.c
	File ..\bfile\*.h
	SetOutPath $INSTDIR\rset
	File ..\rset\*.c
	SetOutPath $INSTDIR\test\api
	File ..\test\api\*.c
	File ..\test\api\*.cfg
	SetOutPath $INSTDIR\win
	File makefile
	File *.nsi
	File *.txt
SectionEnd

; begin uninstall settings/section
UninstallText "This will uninstall Zebra ${VERSION} from your system"

Section Uninstall
; add delete commands to delete whatever files/registry keys/etc you installed here.
	Delete "$INSTDIR\uninst.exe"
	DeleteRegKey HKLM "SOFTWARE\Index Data\Zebra"
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Zebra"
	ExecWait '"$INSTDIR\bin\zebrasrv" -remove'
	RMDir /r "$SMPROGRAMS\Index Data\Zebra"
	RMDir /r $INSTDIR
        IfFileExists $INSTDIR 0 Removed 
		MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "Note: $INSTDIR could not be removed."
Removed:
SectionEnd
; eof
