; $Id: zebra.nsi,v 1.20.2.5 2004-08-20 12:32:01 adam Exp $

!define VERSION "1.3.18"

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
	File ..\bin\*.exe
	File ..\bin\iconv.dll
	File ..\bin\zlib.dll
	File ..\bin\libxml2.dll
	File ..\bin\yaz.dll
	File ..\bin\libexpat.dll
	File c:\winnt\system32\msvcr71.dll
SectionEnd

Section "Zebra Development"
	SectionIn 1
	SetOutPath $INSTDIR\include
	File ..\include\*.h
	SetOutPath $INSTDIR\lib
	File ..\lib\*.lib
SectionEnd

Section "Zebra Documentation"
	SectionIn 1 2
	SetOutPath $INSTDIR\doc
	File ..\doc\*.html
	File ..\doc\*.png
	File ..\doc\*.pdf
	File ..\doc\*.xml
	File ..\doc\*.in
	File ..\doc\*.xsl
	SetOutPath "$SMPROGRAMS\Index Data\Zebra\"
	CreateShortCut "$SMPROGRAMS\Index Data\Zebra\HTML Documentation.lnk" \
                 "$INSTDIR\doc\zebra.html"
	CreateShortCut "$SMPROGRAMS\Index Data\Zebra\PDF Documentaion.lnk" \
                 "$INSTDIR\doc\zebra.pdf"
SectionEnd

Section "Zebra Examples"
	SectionIn 1 2

	SetOutPath $INSTDIR

	File /r ..\test
	File /r ..\examples
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
	SetOutPath $INSTDIR\isam
	File ..\isam\*.c
	File ..\isam\*.h
	SetOutPath $INSTDIR\isamc
	File ..\isamc\*.c
	File ..\isamc\*.h
	SetOutPath $INSTDIR\isamb
	File ..\isamb\*.c
	SetOutPath $INSTDIR\data1
	File ..\data1\*.c
	SetOutPath $INSTDIR\recctrl
	File ..\recctrl\*.c
	File ..\recctrl\*.h
	SetOutPath $INSTDIR\dict
	File ..\dict\*.c
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
