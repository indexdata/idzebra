; $Id: zebra.nsi,v 1.4 2002-09-09 09:36:24 adam Exp $

!define VERSION "1.3.2"

Name "Zebra"
Caption "Index Data Yebra ${VERSION} Setup"
OutFile "idzebra_${VERSION}.exe"

LicenseText "You must read the following license before installing:"
LicenseData license.txt

ComponentText "This will install Zebra on your computer:"
InstType "Full (w/ Source)"
InstType "Lite (w/o Source)"

; Some default compiler settings (uncomment and change at will):
; SetCompress auto ; (can be off or force)
; SetDatablockOptimize on ; (can be off)
; CRCCheck on ; (can be off)
; AutoCloseWindow false ; (can be true for the window go away automatically at end)
; ShowInstDetails hide ; (can be show to have them shown, or nevershow to disable)
; SetDateSave off ; (can be on to have files restored to their orginal date)

InstallDir "$PROGRAMFILES\Zebra"
InstallDirRegKey HKLM "SOFTWARE\Index Data\Zebra" ""
DirShow show ; (make this hide to not let the user change it)
DirText "Select the directory to install Zebra in:"

Section "" ; (default section)
	SetOutPath "$INSTDIR"
	; add files / whatever that need to be installed here.
	WriteRegStr HKLM "SOFTWARE\Index Data\Zebra" "" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Zebra" "DisplayName" "Zebra ${VERSION} (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Zebra" "UninstallString" '"$INSTDIR\uninst.exe"'
	; write out uninstaller
	WriteUninstaller "$INSTDIR\uninst.exe"
	SetOutPath $SMPROGRAMS\Zebra
 	CreateShortCut "$SMPROGRAMS\Zebra\Zebra Program Directory.lnk" \
                 "$INSTDIR"
	WriteINIStr "$SMPROGRAMS\Zebra\Zebra Home page.url" \
              "InternetShortcut" "URL" "http://www.indexdata.dk/zebra/"
	CreateShortCut "$SMPROGRAMS\Zebra\Uninstall Zebra.lnk" \
		"$INSTDIR\uninst.exe"
	SetOutPath $INSTDIR
	File ..\LICENSE.zebra
	File ..\README
	SetOutPath $INSTDIR
	File /r ..\tab
SectionEnd ; end of default section

Section "Zebra Runtime"
	SectionIn 1 2
	IfFileExists "$INSTDIR\bin\zebrasrv.exe" 0 Noservice
	ExecWait '"$INSTDIR\bin\zebrasrv.exe" -remove'
Noservice:
	SetOutPath $INSTDIR\bin
	File ..\bin\*.exe
	File ..\bin\*.dll
	SetOutPath $SMPROGRAMS\Zebra
	SetOutPath $SMPROGRAMS\Zebra\Server
 	CreateShortCut "$SMPROGRAMS\Zebra\Server\Server on console on port 9999.lnk" \
                 "$INSTDIR\bin\zebrasrv.exe"
  	CreateShortCut "$SMPROGRAMS\Zebra\Server\Install Z39.50 service on port 210.lnk" \
                  "$INSTDIR\bin\zebrasrv.exe" '-installa tcp:@:210'
 	CreateShortCut "$SMPROGRAMS\Zebra\Server\Remove Z39.50 service.lnk" \
                 "$INSTDIR\bin\zebrasrv.exe" '-remove'
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
	SetOutPath $INSTDIR
	File /r ..\doc
	SetOutPath $SMPROGRAMS\Zebra
	CreateShortCut "$SMPROGRAMS\Zebra\HTML Documentation.lnk" \
                 "$INSTDIR\doc\zebra.html"
	CreateShortCut "$SMPROGRAMS\Zebra\PDF Documentaion.lnk" \
                 "$INSTDIR\doc\zebra.pdf"
SectionEnd

Section "Zebra Examples"
	SectionIn 1 2

	SetOutPath $INSTDIR

	File /r ..\test
;	File /r ..\examples
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
	RMDir /r $SMPROGRAMS\Zebra
	RMDir /r $INSTDIR
        IfFileExists $INSTDIR 0 Removed 
		MessageBox MB_OK|MB_ICONEXCLAMATION \
                 "Note: $INSTDIR could not be removed."
Removed:
SectionEnd
; eof
