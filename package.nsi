; NSIS Installer Script for ElegooSlicer
; Supports: English, Chinese Simplified, Thai

!define LIBRARY_X64
!define PRODUCT_NAME "ElegooSlicer"
!define PRODUCT_PUBLISHER "Shenzhen Elegoo Technology Co.,Ltd"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\elegoo-slicer.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

!define PRODUCT_UNINST_KEY_32 "Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

SetCompressor lzma

VIProductVersion "${PRODUCT_VERSION}"
VIAddVersionKey "ProductName" "ElegooSlicer"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "CompanyName" "Shenzhen Elegoo Technology Co., Ltd"
VIAddVersionKey "FileDescription" "ElegooSlicer"
VIAddVersionKey "LegalCopyright" ""

; ------ MUI Modern UI ------
!include "MUI.nsh"
!include "nsProcess.nsh"
!include "WordFunc.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON ".\resources\images\ElegooSlicer.ico"
!define MUI_UNICON ".\resources\images\ElegooSlicer.ico"

; Language selection dialog settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!define MUI_LICENSEPAGE_CHECKBOX
!insertmacro MUI_PAGE_LICENSE ".\LICENSE.txt"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Start menu page
var ICONS_GROUP
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "ElegooSlicer"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"

; Language IDs
!define LANG_ENGLISH 1033
!define LANG_CHINESE_SIMPLIFIED 2052
!define LANG_THAI 1054

; Start menu checkbox text
LangString MUI_STARTMENUPAGE_TEXT ${LANG_ENGLISH} "Do not create shortcuts"
LangString MUI_STARTMENUPAGE_TEXT ${LANG_CHINESE_SIMPLIFIED} "Do not create shortcuts"
LangString MUI_STARTMENUPAGE_TEXT ${LANG_THAI} "ไม่ต้องสร้างทางลัด"

!define MUI_STARTMENUPAGE_TEXT_CHECKBOX $(MUI_STARTMENUPAGE_TEXT)
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
; Install files page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION "RunMainApp"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Thai"

; Reserve files
!insertmacro MUI_RESERVEFILE_LANGDLL
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile ".\build\ElegooSlicer_Windows_Installer_V${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES64\ElegooSlicer"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  ${WordFind} "$INSTDIR" "ElegooSlicer" "E+1{" $R0
  IfErrors notfound end
  notfound:
    StrCpy $INSTDIR $INSTDIR\ElegooSlicer
  end:

  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  File /r "${INSTALL_PATH}\*.*"

  ; Create start menu shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\ElegooSlicer.lnk" "$INSTDIR\elegoo-slicer.exe"
  CreateShortCut "$DESKTOP\ElegooSlicer.lnk" "$INSTDIR\elegoo-slicer.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -AdditionalIcons
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\elegoo-slicer.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\elegoo-slicer.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

; Variables
Var UNINSTALL_PROG
Var OLD_VER
Var UNINSTALL_PROG_32
Var OLD_VER_32
Var PUBLIC_DESKTOP_PATH
Var OLD_PATH
Var VER_INFO
Var UNINSTALL_PATH

; Localized strings - Uninstall success
LangString TXT_UNINSTALL_SUCCESS ${LANG_ENGLISH} "$(^Name) has been successfully removed from your computer."
LangString TXT_UNINSTALL_SUCCESS ${LANG_CHINESE_SIMPLIFIED} "$(^Name) has been successfully removed from your computer."
LangString TXT_UNINSTALL_SUCCESS ${LANG_THAI} "$(^Name) ถูกถอนการติดตั้งจากคอมพิวเตอร์ของคุณเรียบร้อยแล้ว"

; Localized strings - Program running (install)
LangString TXT_PROG_RUNNING ${LANG_ENGLISH} "The installer has detected that ${PRODUCT_NAME} is running.$\nClick 'OK' to force close ${PRODUCT_NAME} and continue the installation.$\nClick 'Cancel' to exit the installer."
LangString TXT_PROG_RUNNING ${LANG_CHINESE_SIMPLIFIED} "The installer has detected that ${PRODUCT_NAME} is running.$\nClick 'OK' to force close ${PRODUCT_NAME} and continue the installation.$\nClick 'Cancel' to exit the installer."
LangString TXT_PROG_RUNNING ${LANG_THAI} "ตัวติดตั้งตรวจพบว่า ${PRODUCT_NAME} กำลังทำงานอยู่$\nคลิก 'ตกลง' เพื่อบังคับปิดและดำเนินการติดตั้งต่อ$\nคลิก 'ยกเลิก' เพื่อออกจากตัวติดตั้ง"

; Localized strings - Old version detected
LangString TXT_OLD_VERSION ${LANG_ENGLISH} "Detected version $VER_INFO. Do you want to uninstall it and continue with the installation?"
LangString TXT_OLD_VERSION ${LANG_CHINESE_SIMPLIFIED} "Detected version $VER_INFO. Do you want to uninstall it and continue with the installation?"
LangString TXT_OLD_VERSION ${LANG_THAI} "พบเวอร์ชัน $VER_INFO คุณต้องการถอนการติดตั้งและดำเนินการติดตั้งต่อหรือไม่?"

; Localized strings - Confirm uninstall
LangString TXT_CONFIRM_UNINSTALL ${LANG_ENGLISH} "Do you really want to completely remove $(^Name)?"
LangString TXT_CONFIRM_UNINSTALL ${LANG_CHINESE_SIMPLIFIED} "Do you really want to completely remove $(^Name)?"
LangString TXT_CONFIRM_UNINSTALL ${LANG_THAI} "คุณแน่ใจหรือไม่ว่าต้องการถอนการติดตั้ง $(^Name) ทั้งหมด?"


Function UninstallOldVersion
  Exch $0
  ${If} $0 != ""
    ${WordReplace} "$0" "$\"" "" "+" $0
    StrCpy $UNINSTALL_PATH $0
    StrCpy $OLD_PATH $0 -13
    ExecWait '"$UNINSTALL_PATH" /S _?=$OLD_PATH' $0
    DetailPrint "Uninstall.exe returned $0"
    Delete "$UNINSTALL_PATH"
    RMDir /r $OLD_PATH
    ReadRegStr $PUBLIC_DESKTOP_PATH HKLM "Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" "Common Desktop"
    Delete "$PUBLIC_DESKTOP_PATH\ElegooSlicer.lnk"
  ${EndIf}
FunctionEnd


Function .onInit
  SetRegView 64

  ; Auto-detect language based on system locale
  System::Call 'Kernel32::GetUserDefaultUILanguage() i.r0'
  ${If} $0 == ${LANG_THAI}
      StrCpy $LANGUAGE ${LANG_THAI}
  ${ElseIf} $0 == ${LANG_CHINESE_SIMPLIFIED}
      StrCpy $LANGUAGE ${LANG_CHINESE_SIMPLIFIED}
  ${Else}
      StrCpy $LANGUAGE ${LANG_ENGLISH}
  ${EndIf}

  ReadRegStr $UNINSTALL_PROG ${PRODUCT_UNINST_ROOT_KEY} ${PRODUCT_UNINST_KEY} "UninstallString"
  ReadRegStr $OLD_VER ${PRODUCT_UNINST_ROOT_KEY} ${PRODUCT_UNINST_KEY} "DisplayVersion"
  ReadRegStr $UNINSTALL_PROG_32 ${PRODUCT_UNINST_ROOT_KEY} ${PRODUCT_UNINST_KEY_32} "UninstallString"
  ReadRegStr $OLD_VER_32 ${PRODUCT_UNINST_ROOT_KEY} ${PRODUCT_UNINST_KEY_32} "DisplayVersion"

  nsProcess::_FindProcess "elegoo-slicer.exe"
  Pop $R0
  ${If} $R0 == 0
    MessageBox MB_OKCANCEL|MB_ICONSTOP $(TXT_PROG_RUNNING) /SD IDOK IDOK kill_and_continue IDCANCEL abort_install
  ${EndIf}

  StrCpy $VER_INFO ""

  ${If} $UNINSTALL_PROG != ""
    StrCpy $VER_INFO $OLD_VER
  ${ElseIf} $UNINSTALL_PROG_32 != ""
    StrCpy $VER_INFO $OLD_VER_32
  ${Else}
    goto done
  ${EndIf}

  MessageBox MB_YESNO|MB_ICONQUESTION $(TXT_OLD_VERSION) /SD IDYES IDYES uninstall_old_version IDNO abort_install

  abort_install:
    Abort

  kill_and_continue:
    nsProcess::_KillProcess "elegoo-slicer.exe"
    Pop $R0
    Sleep 1000
    goto uninstall_old_version

  uninstall_old_version:
    Push $UNINSTALL_PROG
    Call UninstallOldVersion
    Push $UNINSTALL_PROG_32
    Call UninstallOldVersion
    ${If} $OLD_PATH != ""
      StrCpy $INSTDIR $OLD_PATH
    ${EndIf}

  done:

FunctionEnd

Function RunMainApp
  System::Call 'shell32::ShellExecute(i 0, t"open", t"explorer.exe", t" /e,elegoo-slicer.exe", t"$INSTDIR\\", i 0)'
FunctionEnd

;*********************************
;  Uninstaller Section
;*********************************

Section Uninstall
  SetRegView 64
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\Website.lnk"
  Delete "$DESKTOP\ElegooSlicer.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\ElegooSlicer.lnk"
  RMDir "$SMPROGRAMS\$ICONS_GROUP"

  RMDir /r "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true

SectionEnd

Function un.onInit
  System::Call 'Kernel32::GetUserDefaultUILanguage() i.r0'
  ${If} $0 == ${LANG_THAI}
      StrCpy $LANGUAGE ${LANG_THAI}
  ${ElseIf} $0 == ${LANG_CHINESE_SIMPLIFIED}
      StrCpy $LANGUAGE ${LANG_CHINESE_SIMPLIFIED}
  ${Else}
      StrCpy $LANGUAGE ${LANG_ENGLISH}
  ${EndIf}

  nsProcess::_FindProcess "elegoo-slicer.exe"
  Pop $R0
  ${If} $R0 = 0
    MessageBox MB_OKCANCEL|MB_ICONSTOP $(TXT_PROG_RUNNING) /SD IDOK IDOK kill_and_continue IDCANCEL abort_install
  ${EndIf}
  Goto done

  abort_install:
    Abort

  kill_and_continue:
    nsProcess::_KillProcess "elegoo-slicer.exe"
    Pop $R0
    Sleep 1000

  done:
    MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 $(TXT_CONFIRM_UNINSTALL) /SD IDYES IDYES continue_uninstall IDNO abort_uninstall

  abort_uninstall:
    Abort

  continue_uninstall:

FunctionEnd


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK $(TXT_UNINSTALL_SUCCESS) /SD IDOK
FunctionEnd
