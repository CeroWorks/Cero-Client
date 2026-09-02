!define APP_NAME        "CeroClient"
!define APP_VERSION     "3.2.17F"
!define APP_PUBLISHER   "CeroClient"
!define APP_EXE         "ceroclient-bootstrapper.exe"
!define APP_ICON        "assets\favicon.ico"
!define INSTALL_DIR     "$LOCALAPPDATA\CeroClient"
!define CDN_URL         "https://github.com/CeroWorks/Cero-Client/releases/latest/download/CeroClient-bootstrapper-windows-x86_64.zip"

!include "MUI2.nsh"

Name "${APP_NAME} ${APP_VERSION}"
OutFile "CeroClient-Setup-${APP_VERSION}.exe"
InstallDir "${INSTALL_DIR}"
RequestExecutionLevel user
Unicode true
SetCompressor /SOLID lzma

VIProductVersion "2.4.1.0"
VIAddVersionKey "ProductName"     "${APP_NAME}"
VIAddVersionKey "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey "CompanyName"     "${APP_PUBLISHER}"
VIAddVersionKey "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey "FileVersion"     "${APP_VERSION}"
VIAddVersionKey "LegalCopyright"  "© ${APP_PUBLISHER}"

!define MUI_ICON   "${APP_ICON}"
!define MUI_UNICON "${APP_ICON}"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Lancer ${APP_NAME}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "French"

Section "Install"
    SetOutPath "$INSTDIR"

    File "/oname=icon.ico" "${APP_ICON}"

    DetailPrint "Téléchargement du bootstrapper..."
    nsExec::ExecToStack 'curl.exe --ssl-no-revoke --silent --fail --show-error -L -o "$INSTDIR\bootstrapper.zip" "${CDN_URL}"'
    Pop $0
    Pop $1

    StrCmp $0 "0" download_ok
        MessageBox MB_ICONSTOP "Échec du téléchargement du bootstrapper :$\n$1"
        Abort
    download_ok:

    DetailPrint "Extraction du bootstrapper..."
    ZipDLL::extractall "$INSTDIR\bootstrapper.zip" "$INSTDIR"
    Pop $0
    StrCmp $0 "success" extract_ok
        MessageBox MB_ICONSTOP "Échec de l'extraction du bootstrapper :$\n$0"
        Abort
    extract_ok:

    Delete "$INSTDIR\bootstrapper.zip"

    IfFileExists "$INSTDIR\${APP_EXE}" exe_present
        MessageBox MB_ICONSTOP "${APP_EXE} introuvable après extraction."
        Abort
    exe_present:

    CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\icon.ico"
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\icon.ico"
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\uninstaller.lnk" "$INSTDIR\uninstall.exe"

    WriteUninstaller "$INSTDIR\uninstall.exe"

    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "DisplayName"     "${APP_NAME}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "Publisher"       "${APP_PUBLISHER}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "DisplayIcon"     "$INSTDIR\icon.ico"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
        "NoRepair" 1
SectionEnd

Section "Uninstall"
    Delete "$DESKTOP\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\uninstaller.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"

    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\icon.ico"
    Delete "$INSTDIR\uninstall.exe"

    ; RMDir /r "$APPDATA\.ceroclient"

    RMDir "$INSTDIR"

    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd
