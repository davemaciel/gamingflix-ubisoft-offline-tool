; ============================================================
;  GamingFlix Ubisoft Offline Tool - Instalador (NSIS + MUI2)
;  Assistente com: boas-vindas, aceite de termos, escolha de
;  pasta, progresso de instalacao, atalhos e finalizacao.
;  Build:  makensis installer/installer.nsi
; ============================================================

Unicode true
SetCompressor /SOLID lzma

!include "MUI2.nsh"

; ---- Metadados ----
!define APP_NAME     "GamingFlix Ubisoft Offline Tool"
!define APP_SHORT    "GamingFlix Ubisoft Offline"
!define APP_EXE      "GamingFlix-Ubisoft-Offline-Tool.exe"
!define APP_VERSION  "1.1.0.0"
!define APP_PUBLISHER "GamingFlix"
!define APP_URL      "https://gamingflix.space"
!define APP_REPO     "https://github.com/davemaciel/gamingflix-ubisoft-offline-tool"

Name "${APP_NAME}"
OutFile "..\dist\GamingFlix-Ubisoft-Offline-Setup.exe"
InstallDir "$PROGRAMFILES64\${APP_SHORT}"
InstallDirRegKey HKLM "Software\${APP_SHORT}" "InstallDir"
RequestExecutionLevel admin
ShowInstDetails show
ShowUnInstDetails show

; ---- Versao do proprio instalador (ajuda a reduzir alertas) ----
VIProductVersion "${APP_VERSION}"
VIAddVersionKey "ProductName"     "${APP_NAME}"
VIAddVersionKey "CompanyName"     "${APP_PUBLISHER}"
VIAddVersionKey "FileDescription" "Instalador do ${APP_NAME}"
VIAddVersionKey "FileVersion"     "${APP_VERSION}"
VIAddVersionKey "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey "LegalCopyright"  "GamingFlix"
VIAddVersionKey "OriginalFilename" "GamingFlix-Ubisoft-Offline-Setup.exe"

; ---- Aparencia ----
!define MUI_ICON   "..\res\app.ico"
!define MUI_UNICON "..\res\app.ico"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE

; Texto amigavel na tela de boas-vindas
!define MUI_WELCOMEPAGE_TITLE "Instalar o ${APP_NAME}"
!define MUI_WELCOMEPAGE_TEXT "Este assistente vai instalar o GamingFlix Ubisoft Offline Tool no seu computador.$\r$\n$\r$\nEle deixa a Ubisoft Connect em modo offline (via Firewall do Windows) para a sua conta compartilhada nao cair quando outra pessoa entra online.$\r$\n$\r$\nApenas automacoes seguras. Codigo aberto e auditavel.$\r$\n$\r$\nClique em Avancar para continuar."

; Tela de termos
!define MUI_LICENSEPAGE_TEXT_TOP "Leia os termos de uso e a nota de transparencia:"
!define MUI_LICENSEPAGE_TEXT_BOTTOM "Se voce concorda, marque a opcao abaixo e clique em Avancar."
!define MUI_LICENSEPAGE_CHECKBOX
!define MUI_LICENSEPAGE_CHECKBOX_TEXT "Eu li e concordo com os termos"

; Tela final
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Abrir o GamingFlix Ubisoft Offline Tool agora"
!define MUI_FINISHPAGE_LINK "Ver o codigo-fonte no GitHub (transparencia)"
!define MUI_FINISHPAGE_LINK_LOCATION "${APP_REPO}"

; ---- Paginas do instalador ----
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "TERMOS.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; ---- Paginas do desinstalador ----
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "English"

; ============================================================
;  Instalacao
; ============================================================
Section "Aplicativo (obrigatorio)" SecApp
  SectionIn RO
  SetOutPath "$INSTDIR"

  DetailPrint "Copiando o aplicativo..."
  File "..\dist\${APP_EXE}"
  File "TERMOS.txt"

  DetailPrint "Criando atalhos no Menu Iniciar e na Area de Trabalho..."
  CreateDirectory "$SMPROGRAMS\${APP_SHORT}"
  CreateShortCut "$SMPROGRAMS\${APP_SHORT}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
  CreateShortCut "$SMPROGRAMS\${APP_SHORT}\Desinstalar.lnk" "$INSTDIR\uninstall.exe"
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0

  DetailPrint "Registrando para o Adicionar/Remover Programas..."
  WriteRegStr HKLM "Software\${APP_SHORT}" "InstallDir" "$INSTDIR"

  ; Entrada em "Adicionar ou remover programas"
  !define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT}"
  WriteRegStr HKLM "${UNINST_KEY}" "DisplayName"     "${APP_NAME}"
  WriteRegStr HKLM "${UNINST_KEY}" "DisplayIcon"     "$INSTDIR\${APP_EXE}"
  WriteRegStr HKLM "${UNINST_KEY}" "DisplayVersion"  "${APP_VERSION}"
  WriteRegStr HKLM "${UNINST_KEY}" "Publisher"       "${APP_PUBLISHER}"
  WriteRegStr HKLM "${UNINST_KEY}" "URLInfoAbout"    "${APP_URL}"
  WriteRegStr HKLM "${UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair" 1

  DetailPrint "Gerando o desinstalador..."
  WriteUninstaller "$INSTDIR\uninstall.exe"

  DetailPrint "Instalacao concluida com sucesso."
SectionEnd

; ============================================================
;  Desinstalacao
; ============================================================
Section "Uninstall"
  ; remove regras de firewall que o app tenha criado (limpeza)
  DetailPrint "Removendo regras de firewall (se existirem)..."
  nsExec::Exec 'netsh advfirewall firewall delete rule name="GamingFlix_Ubi_Offline"'

  Delete "$INSTDIR\${APP_EXE}"
  Delete "$INSTDIR\TERMOS.txt"
  Delete "$INSTDIR\uninstall.exe"
  RMDir  "$INSTDIR"

  Delete "$SMPROGRAMS\${APP_SHORT}\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${APP_SHORT}\Desinstalar.lnk"
  RMDir  "$SMPROGRAMS\${APP_SHORT}"
  Delete "$DESKTOP\${APP_NAME}.lnk"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT}"
  DeleteRegKey HKLM "Software\${APP_SHORT}"
SectionEnd
