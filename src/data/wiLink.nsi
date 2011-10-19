; CPack install script designed for a nmake build

;--------------------------------
; You must define these values

  !define VERSION "@VERSION@"
  !define INST_DIR "@INST_DIR@"

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER
  Var SV_ALLUSERS
  Var START_MENU
  Var IS_DEFAULT_INSTALLDIR
;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\wiLink"

;--------------------------------
;General

  ;Name and file
  Name "wiLink"
  OutFile "@OUTFILE@"

  ;Set compression
  SetCompressor lzma

;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
  !define MUI_ABORTWARNING
    
;--------------------------------
; path functions

!verbose 3
!include "WinMessages.NSH"
!verbose 4

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall sutff
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

###########################################
#            Utility Functions            #
###########################################
 
Function ConditionalAddToRegisty
  Pop $0
  Pop $1
  StrCmp "$0" "" ConditionalAddToRegisty_EmptyString
    WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\wiLink" \
    "$1" "$0"
    ;MessageBox MB_OK "Set Registry: '$1' to '$0'"
    DetailPrint "Set install registry entry: '$1' to '$0'"
  ConditionalAddToRegisty_EmptyString:
FunctionEnd

;--------------------------------

;--------------------------------
;Pages
  !insertmacro MUI_PAGE_WELCOME

  ;!insertmacro MUI_PAGE_LICENSE "/tmp/COPYING"
  !insertmacro MUI_PAGE_DIRECTORY
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "SHCTX" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Wifirst\wiLink" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER

  !define MUI_FINISHPAGE_RUN "$INSTDIR\bin\wiLink.exe";

  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English" ;first language is the default language
  !insertmacro MUI_LANGUAGE "Albanian"
  !insertmacro MUI_LANGUAGE "Arabic"
  !insertmacro MUI_LANGUAGE "Basque"
  !insertmacro MUI_LANGUAGE "Belarusian"
  !insertmacro MUI_LANGUAGE "Bosnian"
  !insertmacro MUI_LANGUAGE "Breton"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "Estonian"
  !insertmacro MUI_LANGUAGE "Farsi"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Hebrew"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Icelandic"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Irish"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Kurdish"
  !insertmacro MUI_LANGUAGE "Latvian"
  !insertmacro MUI_LANGUAGE "Lithuanian"
  !insertmacro MUI_LANGUAGE "Luxembourgish"
  !insertmacro MUI_LANGUAGE "Macedonian"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Mongolian"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Romanian"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Serbian"
  !insertmacro MUI_LANGUAGE "SerbianLatin"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Slovenian"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Thai"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Ukrainian"
  !insertmacro MUI_LANGUAGE "Welsh"


;--------------------------------
;Installer Sections

Section "-Core installation"
  ;Use the entire tree produced by the INSTALL target.  Keep the
  ;list of directories here in sync with the RMDir commands below.
  SetOutPath "$INSTDIR"
  ExecWait '"taskkill.exe" /F /IM wiLink.exe' $0
  Sleep 1000
  File /r "${INST_DIR}\*.*"
  
  ;Store installation folder
  WriteRegStr SHCTX "Software\Wifirst" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  Push "DisplayName"
  Push "wiLink"
  Call ConditionalAddToRegisty
  Push "DisplayVersion"
  Push "@VERSION@"
  Call ConditionalAddToRegisty
  Push "Publisher"
  Push "Wifirst"
  Call ConditionalAddToRegisty
  Push "UninstallString"
  Push "$INSTDIR\Uninstall.exe"
  Call ConditionalAddToRegisty
  Push "NoRepair"
  Push "1"
  Call ConditionalAddToRegisty
  Push "NoModify"
  Push "1"
  Call ConditionalAddToRegisty
  
  ; Optional registration
  Push "DisplayIcon"
  Push "$INSTDIR\"
  Call ConditionalAddToRegisty
  Push "HelpLink"
  Push ""
  Call ConditionalAddToRegisty
  Push "URLInfoAbout"
  Push ""
  Call ConditionalAddToRegisty
  Push "Contact"
  Push ""
  Call ConditionalAddToRegisty
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\wiLink.lnk" "$INSTDIR\bin\wiLink.exe"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  ; Write special uninstall registry entries
  Push "StartMenu"
  Push "$STARTMENU_FOLDER"
  Call ConditionalAddToRegisty

  !insertmacro MUI_STARTMENU_WRITE_END

  IfSilent 0 +2
  Exec "$INSTDIR\bin\wiLink.exe"

SectionEnd

;--------------------------------
; determine admin versus local install
Function un.onInit

  ClearErrors
  UserInfo::GetName
  IfErrors noLM
  Pop $0
  UserInfo::GetAccountType
  Pop $1
  StrCmp $1 "Admin" 0 +3
    SetShellVarContext all
    ;MessageBox MB_OK 'User "$0" is in the Admin group'
    Goto done
  StrCmp $1 "Power" 0 +3
    SetShellVarContext all
    ;MessageBox MB_OK 'User "$0" is in the Power Users group'
    Goto done
    
  noLM:
    ;Get installation folder from registry if available

  done:
    
FunctionEnd

;--- Add/Remove callback functions: ---
!macro SectionList MacroName
  ;This macro used to perform operation on multiple sections.
  ;List all of your components in following manner here.

!macroend
 
Section -FinishComponents
  ;Removes unselected components and writes component status to registry
  !insertmacro SectionList "FinishSection"
SectionEnd
;--- End of Add/Remove callback functions ---

;--------------------------------
; Component dependencies
Function .onSelChange
  !insertmacro SectionList MaybeSelectionChanged
FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"
  ReadRegStr $START_MENU SHCTX \
   "Software\Microsoft\Windows\CurrentVersion\Uninstall\wiLink" "StartMenu"
  ;MessageBox MB_OK "Start menu is in: $START_MENU"

  ;Remove application data.
  RMDir /r "$LOCALAPPDATA\Wifirst\wiLink"
  RMDir "$LOCALAPPDATA\Wifirst"

  ;Remove files we installed.
  RMDir /r "$INSTDIR\bin"

  ;Remove the uninstaller itself.
  Delete "$INSTDIR\Uninstall.exe"
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\wiLink"

  ;Remove the installation directory if it is empty.
  RMDir "$INSTDIR"

  ; Remove the registry entries.
  DeleteRegValue SHCTX "Software\Microsoft\Windows\CurrentVersion\Run" "wiLink"
  DeleteRegKey SHCTX "Software\Wifirst\wiLink"
  DeleteRegKey /ifempty SHCTX "Software\Wifirst"

  ; Removes all optional components
  !insertmacro SectionList "RemoveSection"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
    
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\wiLink.lnk"

  
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  startMenuDeleteLoop:
    ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
  
    StrCmp "$MUI_TEMP" "$SMPROGRAMS" startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  ; If the user changed the shortcut, then untinstall may not work. This should
  ; try to fix it.
  StrCpy $MUI_TEMP "$START_MENU"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"

  
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  secondStartMenuDeleteLoop:
    ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors secondStartMenuDeleteLoopDone
  
    StrCmp "$MUI_TEMP" "$SMPROGRAMS" secondStartMenuDeleteLoopDone secondStartMenuDeleteLoop
  secondStartMenuDeleteLoopDone:

  DeleteRegKey /ifempty SHCTX "Software\Wifirst\wiLink"

SectionEnd

;--------------------------------
; determine admin versus local install
; Is install for "AllUsers" or "JustMe"?
; Default to "JustMe" - set to "AllUsers" if admin or on Win9x
; This function is used for the very first "custom page" of the installer.
; This custom page does not show up visibly, but it executes prior to the
; first visible page and sets up $INSTDIR properly...
; Choose different default installation folder based on SV_ALLUSERS...
; "Program Files" for AllUsers, "My Documents" for JustMe...

Function .onInit
  ; Reads components status for registry
  !insertmacro SectionList "InitSection"

  ; check to see if /D has been used to change 
  ; the install directory by comparing it to the 
  ; install directory that is expected to be the
  ; default
  StrCpy $IS_DEFAULT_INSTALLDIR 0
  StrCmp "$INSTDIR" "$PROGRAMFILES\wiLink" 0 +2
    StrCpy $IS_DEFAULT_INSTALLDIR 1
  
  StrCpy $SV_ALLUSERS "JustMe"
  ; if default install dir then change the default
  ; if it is installed for JustMe
  StrCmp "$IS_DEFAULT_INSTALLDIR" "1" 0 +2
    StrCpy $INSTDIR "$DOCUMENTS\wiLink"

  ClearErrors
  UserInfo::GetName
  IfErrors noLM
  Pop $0
  UserInfo::GetAccountType
  Pop $1
  StrCmp $1 "Admin" 0 +3
    SetShellVarContext all
    ;MessageBox MB_OK 'User "$0" is in the Admin group'
    StrCpy $SV_ALLUSERS "AllUsers"
    Goto done
  StrCmp $1 "Power" 0 +3
    SetShellVarContext all
    ;MessageBox MB_OK 'User "$0" is in the Power Users group'
    StrCpy $SV_ALLUSERS "AllUsers"
    Goto done
    
  noLM:
    StrCpy $SV_ALLUSERS "AllUsers"
    ;Get installation folder from registry if available

  done:
  StrCmp $SV_ALLUSERS "AllUsers" 0 +3
    StrCmp "$IS_DEFAULT_INSTALLDIR" "1" 0 +2
      StrCpy $INSTDIR "$PROGRAMFILES\wiLink"

FunctionEnd
