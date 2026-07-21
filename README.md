<h1 align="center">🎮 GamingFlix — Ubisoft Offline Tool</h1>

<p align="center">
  <b>Mantenha a Ubisoft Connect em modo offline e não perca sua sessão quando outra pessoa logar na conta.</b><br>
  <sub>Um clique. Só firewall. Código 100% aberto pra você auditar.</sub>
</p>

<p align="center">
  <img alt="Plataforma" src="https://img.shields.io/badge/Windows-10%20%7C%2011-0d0d12?style=flat-square&logo=windows">
  <img alt="Licença" src="https://img.shields.io/badge/licen%C3%A7a-MIT-22c55e?style=flat-square">
  <img alt="Sem telemetria" src="https://img.shields.io/badge/telemetria-nenhuma-22c55e?style=flat-square">
  <img alt="Código aberto" src="https://img.shields.io/badge/c%C3%B3digo-aberto-eb1735?style=flat-square">
</p>

---

## 😩 O problema

Em contas de jogo **compartilhadas**, quando você está jogando e **outra pessoa entra online** na mesma conta Ubisoft, a Ubisoft **derruba a sua sessão**. Resultado: você é chutado do jogo do nada.

## ✅ A solução

O **GamingFlix Ubisoft Offline Tool** deixa a Ubisoft Connect **presa em modo offline**. Sem conexão, a Ubisoft não detecta o login concorrente e **não te derruba**. Você joga em paz.

> É uma automação simples de conveniência. Nada de mágica, nada escondido.

---

## 🔍 Como funciona (transparência total)

O app faz **UMA coisa** no seu PC: cria (ou remove) **regras de Firewall do Windows** para os executáveis da Ubisoft Connect, usando o comando **oficial** do Windows `netsh advfirewall`.

Ao clicar em **Ativar modo offline**, ele roda o equivalente a:

```bat
netsh advfirewall firewall add rule name="GamingFlix_Ubi_Offline" dir=out action=block program="...\UbisoftConnect.exe" enable=yes
netsh advfirewall firewall add rule name="GamingFlix_Ubi_Offline" dir=in  action=block program="...\UbisoftConnect.exe" enable=yes
```
(para cada executável do Ubisoft Connect: `UbisoftConnect.exe`, `upc.exe`, `UbisoftGameLauncher.exe`, `UbisoftGameLauncher64.exe`, `UplayWebCore.exe`)

Ao clicar em **Desativar modo offline**, ele apenas remove essas regras:

```bat
netsh advfirewall firewall delete rule name="GamingFlix_Ubi_Offline"
```

Toda a lógica de sistema está em [`src/main.c`](src/main.c), nas funções `fw_block()`, `fw_unblock()` e `fw_is_active()`. Leia — são poucas linhas.

---

## 🛡️ O que este app **NÃO** faz

- ❌ **Não** desativa o Windows Update (pode e deve continuar atualizando).
- ❌ **Não** coleta, envia ou armazena nenhum dado seu (zero telemetria, zero rede).
- ❌ **Não** instala serviços, drivers ou tarefas agendadas.
- ❌ **Não** mexe em senhas, contas, arquivos ou em qualquer outro programa.
- ❌ **Não** contém DRM-break, crack, nem nada ilegal — é só firewall.

Precisa de **permissão de administrador** por um único motivo: criar regras de firewall exige admin no Windows. Nada além disso.

---

## 📥 Como usar

### Opção 1 — Instalador (recomendado)

1. Baixe o **`GamingFlix-Ubisoft-Offline-Setup.exe`** na aba **[Releases](../../releases)**.
2. Abra o instalador. Um assistente vai te guiar: **boas-vindas → aceitar os termos → escolher a pasta → instalar**. Ele cria atalhos no **Menu Iniciar** e na **Área de Trabalho**.
3. Ao terminar, marque *"Abrir agora"* ou use o atalho criado.

### Opção 2 — Versão portátil (sem instalar)

1. Baixe o **`GamingFlix-Ubisoft-Offline-Tool.exe`** na aba **[Releases](../../releases)** e abra direto.

### Usando o app

1. O Windows pode pedir permissão de administrador → **Sim** (necessário só pro firewall).
2. Clique em **Ativar modo offline**. **Não precisa abrir o jogo antes** — ative aqui e depois abra a Ubisoft/jogo normalmente (já entra offline).
3. Quando quiser voltar ao normal (atualizar/comprar na Ubisoft), clique em **Desativar modo offline**.

> 💡 Dentro do jogo pode aparecer *"Erro de serviço online"* / *"Not supported"* — **isso é normal** no modo offline. É só continuar e jogar.

### ⚠️ "O Windows protegeu o seu computador" (SmartScreen)

Como o app é novo e **não tem certificado de assinatura pago**, o Windows pode mostrar uma tela azul de aviso. **Isso não é vírus** — é só o app ainda não ter reputação acumulada. Clique em **"Mais informações"** e depois em **"Executar assim mesmo"**. Se preferir, **compile você mesmo** a partir do código (veja abaixo) e rode seu próprio binário.

---

## 🧰 Compilar você mesmo (não confia no `.exe`? compile do código)

Requer **mingw-w64**. Em Linux:

```bash
sudo apt install mingw-w64
./build.sh
# saída: dist/GamingFlix-Ubisoft-Offline-Tool.exe
```

O binário gerado é **exatamente** o que está aqui no repositório. Sem etapas escondidas.

Para gerar também o instalador (opcional, requer **NSIS**):

```bash
sudo apt install nsis
makensis installer/installer.nsi
# saída: dist/GamingFlix-Ubisoft-Offline-Setup.exe
```

---

## ❓ FAQ

**É seguro?** Sim. Só cria regras de firewall reversíveis. Você pode remover tudo pelo próprio app ou pelo Firewall do Windows.

**Vou ser banido?** Não. O app não modifica o jogo nem a Ubisoft; só controla a conexão de rede local dela, um recurso normal do Windows.

**Funciona com qual jogo?** Qualquer jogo que usa Ubisoft Connect. O modo offline vale pra Ubisoft toda.

**Meu antivírus reclamou.** É um `.exe` novo e sem assinatura paga — o SmartScreen/antivírus às vezes alerta por precaução. Como o código é aberto, você pode auditar e/ou compilar você mesmo.

---

## 📄 Licença

[MIT](LICENSE) — use, estude, modifique e compartilhe à vontade.

<p align="center"><sub>Feito com ❤️ pela <a href="https://gamingflix.space">GamingFlix</a> — pra você jogar sem dor de cabeça.</sub></p>
