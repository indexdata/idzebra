!include EnvVarUpdate.nsh
!include version.nsi
!include "MUI.nsh"

!define VS_REDIST_FULL "c:\Program Files (x86)\Microsoft Visual Studio ${VSVER}.0\VC\redist\1033\${VS_REDIST_EXE}"

; For example can be found with regedit:
;  Microsoft Visual C++ 2013 x86 Minimum Runtime
!if "${VSARCH}" = "x64"
; 64-bit
!if "${VSVER}" = "12"
!define VS_REDIST_KEY "SOFTWARE\Classes\Installer\Products\6E8D947A316B3EB3F8F540C548BE2AB9"
!endif
!if "${VSVER}" = "14"
; Microsoft Visual C++ 2015 x64 Minimum Runtime - 14.0.23026
!define VS_REDIST_KEY "SOFTWARE\Classes\Installer\Products\51E9E3D0A7EDB003691F4BFA219B4688"
!endif

InstallDir "$PROGRAMFILES64\Zebra"
!else
; 32-bit
!if "${VSVER}" = "12"
!define VS_REDIST_KEY "SOFTWARE\Classes\Installer\Products\21EE4A31AE32173319EEFE3BD6FDFFE3"
!endif
!if "${VSVER}" = "14"
; Microsoft Visual C++ 2015 x86 Minimum Runtime - 14.0.23026
!define VS_REDIST_KEY "SOFTWARE\Classes\Installer\Products\55E3652ACEB38283D8765E8E9B8E6B57"
!endif

InstallDir "$PROGRAMFILES\Zebra"
!endif

!if "${VSVER}" = "14"
!define VS_REDIST_EXE vc_redist.${VSARCH}.exe
!else
!define VS_REDIST_EXE vcredist_${VSARCH}.exe
!endif

RequestExecutionLevel admin

SetCompressor bzip2

Name "Zebra"
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
	File ..\util\*.tcl
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

Section "Zebra Path"
	SectionIn 1 2
	${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\bin"
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
	${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\bin"
        IfFileExists $INSTDIR 0 Removed
		MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "Note: $INSTDIR could not be removed."
Removed:
SectionEnd
; eof
