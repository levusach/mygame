!define APP_NAME "The World"
!define APP_EXE "world.exe"
!define APP_VERSION "0.1.0"

Name "${APP_NAME}"
OutFile "..\dist\TheWorldSetup.exe"
InstallDir "$LOCALAPPDATA\TheWorld"
RequestExecutionLevel user

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "..\dist\windows\*.*"
  CreateShortcut "$DESKTOP\The World.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortcut "$SMPROGRAMS\The World.lnk" "$INSTDIR\${APP_EXE}"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\The World.lnk"
  Delete "$SMPROGRAMS\The World.lnk"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR"
SectionEnd
