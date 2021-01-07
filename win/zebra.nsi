; This file is part of the Zebra server.

; Using https://nsis.sourceforge.io/EnVar_plug-in
; for path manipulation

Unicode True

!include version.nsi

!include "MUI.nsh"

Name "Zebra"

!include "..\m4\common.nsi"

RequestExecutionLevel admin

SetCompressor bzip2

Caption "Index Data Zebra ${VERSION} Setup"
OutFile "idzebra_${VERSION}.exe"

LicenseText "You must read the following license before installing:"
LicenseData license.txt

ComponentText "This will install Zebra on your computer:"
InstType "Full (w/ Source)"
InstType "Lite (w/o Source)"

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
	File ..\README.md
	SetOutPath $INSTDIR
	File /r ..\tab
SectionEnd ; end of default section

Section "Zebra Runtime"
	SectionIn 1 2
	SetOutPath $INSTDIR\bin
!if "${VS_REDIST_FULL}" != ""
	File "${VS_REDIST_FULL}"
	ReadRegDword $1 HKLM "${VS_REDIST_KEY}" "Version"
	${If} $1 == ""
	  ExecWait '"$INSTDIR\bin\${VS_REDIST_EXE}" /passive /nostart'
	${endif}
	Delete "$INSTDIR\bin\${VS_REDIST_EXE}"
!endif
	File ..\bin\*.exe
	File ..\bin\idzebra.dll
	File ..\bin\libxml2.dll
	File ..\bin\libxslt.dll
	File ..\bin\yaz*.dll
	File ..\bin\icu*.dll
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
	File /nonfatal /r ..\doc\*.html
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
	SetOutPath $INSTDIR
	File ..\IDMETA
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
	SetOutPath $INSTDIR\m4
	File ..\m4\*.m4
	File ..\m4\*.tcl
	File ..\m4\*.nsi
SectionEnd

Section "Zebra Path"
	SectionIn 1 2
	EnVar::SetHKLM
	EnVar::AddValue "PATH" "$INSTDIR\bin"
	Pop $0
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
	EnVar::SetHKLM
	EnVar::DeleteValue "PATH" "$INSTDIR\bin"
	Pop $0
        IfFileExists $INSTDIR 0 Removed
		MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "Note: $INSTDIR could not be removed."
Removed:
SectionEnd
; eof
