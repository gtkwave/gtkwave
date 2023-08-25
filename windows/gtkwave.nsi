Name "${PROJECT_NAME}"
OutFile "${OUT_FILE}"

!include "FileFunc.nsh"
!include "DumpLog.nsh"
!insertmacro GetTime

SetCompress force ; (can be off or force)
CRCCheck on ; (can be off)

LicenseText "Please read and accept the following license:"
LicenseData "COPYING"

BrandingText "GTKWave Installer"

InstallDir "$PROGRAMFILES64\${APPLICATION_NAME}"
InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${APPLICATION_NAME}" ""

; The text to prompt the user to enter a directory
DirText "Choose a folder in which to install ${APPLICATION_NAME}"

; Show details
ShowInstDetails show

Icon "gtkwave_logo.ico"
UninstallIcon "gtkwave_logo.ico"
XPStyle on

Section "Info"
  # Print version info into the installer window
  ${GetTime} "" "L" $0 $1 $2 $3 $4 $5 $6
  DetailPrint "Running installer on $3 $2-$1-$0 $4:$5:$6"
  DetailPrint "Version=${PROJECT_VERSION}"
SectionEnd
  
; optional section
Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\${APPLICATION_NAME}"
  CreateShortCut "$SMPROGRAMS\${APPLICATION_NAME}\Uninstall.lnk" "$INSTDIR\bin\uninst.exe" "" "$INSTDIR\uninst.exe" 0
  CreateShortCut "$SMPROGRAMS\${APPLICATION_NAME}\${APPLICATION_NAME}.lnk" "$INSTDIR\bin\${APPLICATION_NAME}.exe" "" "$INSTDIR\${APPLICATION_NAME}.exe" 0
SectionEnd

Section "" ; (default section)

  SetOutPath $INSTDIR
  
  File /oname=COPYING.txt COPYING
  File README.md
  
  SetOutPath $INSTDIR\bin
  File ${BINDIR}\libintl-8.dll
  File ${BINDIR}\iconv.dll
  #File ${BINDIR}\libpcre-1.dll
  File ${BINDIR}\libpcre2-8-0.dll
  File ${BINDIR}\libgtk-3-0.dll
  File ${BINDIR}\libgdk-3-0.dll
  File ${BINDIR}\libssp-0.dll
  File ${BINDIR}\libgdk_pixbuf-2.0-0.dll
  File ${BINDIR}\libpixman-1-0.dll
  File ${BINDIR}\libffi-8.dll
  File ${BINDIR}\libgio-2.0-0.dll
  File ${BINDIR}\libgnurx-0.dll
  File ${BINDIR}\libcairo-2.dll
  File ${BINDIR}\libcairo-gobject-2.dll
  File ${BINDIR}\zlib1.dll
  File ${BINDIR}\libglib-2.0-0.dll
  File ${BINDIR}\libatk-1.0-0.dll
  File ${BINDIR}\libgobject-2.0-0.dll
  File ${BINDIR}\libgmodule-2.0-0.dll
  File ${BINDIR}\libgthread-2.0-0.dll
  File ${BINDIR}\libpango-1.0-0.dll
  File ${BINDIR}\libfribidi-0.dll
  File ${BINDIR}\libpangocairo-1.0-0.dll
  File ${BINDIR}\libpangoft2-1.0-0.dll
  File ${BINDIR}\libpangowin32-1.0-0.dll
  File ${BINDIR}\libpng16-16.dll
  File ${BINDIR}\libtiff-5.dll
  File ${BINDIR}\libjpeg-62.dll
  File ${BINDIR}\libfontconfig-1.dll
  #File ${BINDIR}\libxml2-2.dll
  File ${BINDIR}\libfreetype-6.dll
  File ${BINDIR}\gdk-pixbuf-query-loaders.exe
  File ${BINDIR}\${LIBGCCDLL}
  File ${BINDIR}\libstdc++-6.dll
  File ${BINDIR}\libwinpthread-1.dll
  File ${BINDIR}\libexpat-1.dll
  File ${BINDIR}\libbz2-1.dll
  File ${BINDIR}\libharfbuzz-0.dll
  File ${BINDIR}\libharfbuzz-icu-0.dll
  File ${BINDIR}\libharfbuzz-subset-0.dll
  File ${BINDIR}\gspawn-win64-helper.exe
  File ${BINDIR}\gdbus.exe
  File ${BINDIR}\libepoxy-0.dll
  File ${BUILDROOT}\src\*.exe
  File ${BUILDROOT}\src\helpers\*.exe
  
  
  SetOutPath "$INSTDIR"
  File /r ${SYSCONFDIR}
  SetOutPath $INSTDIR\lib\gdk-pixbuf-2.0\2.10.0\loaders
  File ${LIBDIR}\gdk-pixbuf-2.0\2.10.0\loaders\*
  SetOutPath $INSTDIR\lib\gdk-pixbuf-2.0\2.10.0
  File ${LIBDIR}\gdk-pixbuf-2.0\2.10.0\loaders.cache
  
  SetOutPath $INSTDIR\share
  File /r ${DATADIR}\themes
  SetOutPath $INSTDIR\share\icons
  File /r ${DATADIR}\icons\Adwaita
  
  SetOutPath $INSTDIR\share\glib-2.0
  File /r ${DATADIR}\glib-2.0\schemas
  
  # Build the gdk-pixbuf.loaders file automatically
  ExpandEnvStrings $0 %COMSPEC%
  nsExec::ExecToStack '"$0" /C ""$INSTDIR\bin\gdk-pixbuf-query-loaders" > "$INSTDIR\lib\gdk-pixbuf-2.0\2.10.0\loaders.cache""'
  
  ; Set up association with .gtkw files
  DeleteRegKey HKCR ".gtkw"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.gtkw"
  WriteRegStr HKCR ".gtkw" "" "GtkwFile"
  DeleteRegKey HKCR "GtkwFile"
  DeleteRegKey HKCR "Gtkw File"
  WriteRegStr HKCR "GtkwFile" "" "Gtkw File"
  WriteRegStr HKCR "GtkwFile\DefaultIcon" "" "$INSTDIR\bin\gtkwave.exe,1"
  WriteRegStr HKCR "GtkwFile\shell" "" "open"
  WriteRegStr HKCR "GtkwFile\shell\open\command" "" '$INSTDIR\bin\gtkwave.exe "%1"'
  
  ; Set up association with .vcd files
  DeleteRegKey HKCR ".vcd"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.vcd"
  WriteRegStr HKCR ".vcd" "" "VcdFile"
  DeleteRegKey HKCR "VcdFile"
  DeleteRegKey HKCR "Vcd File"
  WriteRegStr HKCR "VcdFile" "" "Vcd File"
  WriteRegStr HKCR "VcdFile\DefaultIcon" "" "$INSTDIR\bin\gtkwave.exe,1"
  WriteRegStr HKCR "VcdFile\shell" "" "open"
  WriteRegStr HKCR "VcdFile\shell\open\command" "" '$INSTDIR\bin\gtkwave.exe "%1"'
  
  ; Set up association with .fst files
  DeleteRegKey HKCR ".fst"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.fst"
  WriteRegStr HKCR ".fst" "" "FstFile"
  DeleteRegKey HKCR "FstFile"
  DeleteRegKey HKCR "Fst File"
  WriteRegStr HKCR "FstFile" "" "Fst File"
  WriteRegStr HKCR "FstFile\DefaultIcon" "" "$INSTDIR\bin\gtkwave.exe,1"
  WriteRegStr HKCR "FstFile\shell" "" "open"
  WriteRegStr HKCR "FstFile\shell\open\command" "" '$INSTDIR\bin\gtkwave.exe "%1"'


  System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
  
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\${APPLICATION_NAME}" "" "$INSTDIR"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPLICATION_NAME}" "DisplayName" "${APPLICATION_NAME} (remove only)"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPLICATION_NAME}" "UninstallString" '"$INSTDIR\bin\uninst.exe"'
  ; write out uninstaller
  WriteUninstaller "$INSTDIR\bin\uninst.exe"

  # Copy the install file
  StrCpy $0 "$INSTDIR\install-gtkwave.log"
  Push $0
  Call DumpLog
SectionEnd ; end of default section

; begin uninstall settings/section
UninstallText "This will uninstall ${APPLICATION_NAME} from your system"

Section Uninstall
  ; add delete commands to delete whatever files/registry keys/etc you installed here.
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\${APPLICATION_NAME}" ""
  DetailPrint "Deleting from $0"
  
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${APPLICATION_NAME}"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${APPLICATION_NAME}"
  SetOutPath "$TEMP"
  RMDir /r "$0"
  
  RMDir /r "$SMPROGRAMS\${APPLICATION_NAME}"
SectionEnd ; end of uninstall section

; eof
