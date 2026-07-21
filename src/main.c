/*
 * GamingFlix Ubisoft Offline Tool
 * -------------------------------
 * Coloca a Ubisoft Connect em MODO OFFLINE bloqueando os executaveis dela
 * no Firewall do Windows. Isso evita que a sua conta seja derrubada quando
 * outra pessoa entra online na mesma conta compartilhada.
 *
 * TRANSPARENCIA: este app so faz UMA coisa no sistema — cria (ou remove)
 * regras de Firewall do Windows para os executaveis da Ubisoft, usando o
 * comando oficial "netsh advfirewall". Nao instala nada, nao coleta dados,
 * nao mexe em outros programas e NAO desativa o Windows Update.
 * Todas as acoes de sistema estao nas funcoes fw_block()/fw_unblock()/fw_is_active().
 *
 * Codigo aberto: https://github.com/  (GamingFlix)
 * Licenca: MIT
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "logo_data.h"   /* logo GamingFlix embutido (BGRA premultiplicado) */

#define IDI_APPICON 101

/* ----- Identidade / constantes ----- */
#define APP_TITLE   "GamingFlix Ubisoft Offline Tool"
#define RULE_NAME   "GamingFlix_Ubi_Offline"
#define SITE_URL    "https://gamingflix.space"
#define REPO_URL    "https://github.com/davemaciel/gamingflix-ubisoft-offline-tool"
#define APP_VERSION "v1.1"

/* ----- Cores (GamingFlix: dark + vermelho) ----- */
#define C_BG        RGB(0x0d, 0x0d, 0x12)
#define C_CARD      RGB(0x1b, 0x1b, 0x24)
#define C_CARD2     RGB(0x14, 0x14, 0x1b)
#define C_LINE      RGB(0x2a, 0x2a, 0x36)
#define C_TEXT      RGB(0xf5, 0xf5, 0xf7)
#define C_MUTED     RGB(0x9a, 0x9a, 0xa8)
#define C_FAINT     RGB(0x6a, 0x6a, 0x78)
#define C_RED       RGB(0xeb, 0x17, 0x35)
#define C_RED_Hi    RGB(0xff, 0x2d, 0x49)
#define C_GREEN     RGB(0x22, 0xc5, 0x5e)
#define C_GREEN_Hi  RGB(0x4a, 0xde, 0x80)
#define C_AMBER     RGB(0xf5, 0x9e, 0x0b)
#define C_AMBER_Hi  RGB(0xfb, 0xbf, 0x24)

/* ----- Layout ----- */
#define WIN_W 430
#define WIN_H 560

/* ----- Estado ----- */
static BOOL  g_offline = FALSE;   /* TRUE = modo offline ativo (Ubisoft bloqueada) */
static BOOL  g_busy    = FALSE;
static HFONT g_fBrand, g_fKick, g_fPill, g_fSub, g_fBtn, g_fHelp, g_fChip, g_fFoot, g_fTag;

/* Retangulos clicaveis (preenchidos no paint) */
static RECT r_primary, r_chipHow, r_chipGit, r_close, r_min;

/* ============================================================
 *  NUCLEO: acoes no Firewall do Windows (o que o app realmente faz)
 * ============================================================ */

/* Executa um comando de forma silenciosa (sem abrir janela). Retorna o exit code. */
static DWORD run_hidden(const char *cmd)
{
    char buf[1024];
    _snprintf(buf, sizeof(buf), "cmd.exe /c %s", cmd);
    buf[sizeof(buf) - 1] = 0;

    STARTUPINFOA si; PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, buf, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        return (DWORD)-1;

    WaitForSingleObject(pi.hProcess, 20000);
    DWORD code = 0; GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    return code;
}

/* Descobre a pasta de instalacao do Ubisoft Connect (registro + fallbacks). */
static BOOL get_ubisoft_dir(char *out, DWORD outSize)
{
    HKEY hk; DWORD type, sz;
    const char *subkeys[] = {
        "SOFTWARE\\WOW6432Node\\Ubisoft\\Launcher",
        "SOFTWARE\\Ubisoft\\Launcher"
    };
    int i;
    for (i = 0; i < 2; i++) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkeys[i], 0, KEY_READ, &hk) == ERROR_SUCCESS) {
            sz = outSize; type = 0;
            if (RegQueryValueExA(hk, "InstallDir", NULL, &type, (LPBYTE)out, &sz) == ERROR_SUCCESS
                && (type == REG_SZ || type == REG_EXPAND_SZ)) {
                RegCloseKey(hk);
                /* remove barra final, se houver */
                size_t l = strlen(out);
                if (l && (out[l-1] == '\\' || out[l-1] == '/')) out[l-1] = 0;
                return TRUE;
            }
            RegCloseKey(hk);
        }
    }
    /* fallbacks comuns */
    const char *fb[] = {
        "C:\\Program Files (x86)\\Ubisoft\\Ubisoft Game Launcher",
        "C:\\Program Files\\Ubisoft\\Ubisoft Game Launcher"
    };
    for (i = 0; i < 2; i++) {
        if (GetFileAttributesA(fb[i]) != INVALID_FILE_ATTRIBUTES) {
            strncpy(out, fb[i], outSize - 1); out[outSize - 1] = 0;
            return TRUE;
        }
    }
    return FALSE;
}

/* Adiciona regras de bloqueio (entrada + saida) para um executavel, se existir. */
static void block_program(const char *fullPath)
{
    char cmd[1200];
    if (GetFileAttributesA(fullPath) == INVALID_FILE_ATTRIBUTES) return; /* nao existe, ignora */

    _snprintf(cmd, sizeof(cmd),
        "netsh advfirewall firewall add rule name=\"%s\" dir=out action=block program=\"%s\" enable=yes",
        RULE_NAME, fullPath);
    run_hidden(cmd);

    _snprintf(cmd, sizeof(cmd),
        "netsh advfirewall firewall add rule name=\"%s\" dir=in action=block program=\"%s\" enable=yes",
        RULE_NAME, fullPath);
    run_hidden(cmd);
}

/* Ativa o modo offline: bloqueia os executaveis da Ubisoft no firewall. */
static BOOL fw_block(void)
{
    char dir[MAX_PATH]; char path[MAX_PATH];
    if (!get_ubisoft_dir(dir, sizeof(dir))) return FALSE;

    /* executaveis conhecidos do Ubisoft Connect */
    const char *exes[] = {
        "UbisoftConnect.exe",
        "upc.exe",
        "UbisoftGameLauncher.exe",
        "UbisoftGameLauncher64.exe",
        "UplayWebCore.exe"
    };
    int i;
    for (i = 0; i < 5; i++) {
        _snprintf(path, sizeof(path), "%s\\%s", dir, exes[i]);
        block_program(path);
    }

    /* fecha a Ubisoft para que ela reabra ja bloqueada (offline) */
    run_hidden("taskkill /F /IM UbisoftConnect.exe /IM upc.exe "
               "/IM UbisoftGameLauncher.exe /IM UbisoftGameLauncher64.exe "
               "/IM UplayWebCore.exe >nul 2>&1");
    return TRUE;
}

/* Desativa o modo offline: remove as regras de firewall do app. */
static BOOL fw_unblock(void)
{
    char cmd[256];
    _snprintf(cmd, sizeof(cmd),
        "netsh advfirewall firewall delete rule name=\"%s\"", RULE_NAME);
    run_hidden(cmd);
    return TRUE;
}

/* Verifica se as regras do app existem (modo offline ativo). */
static BOOL fw_is_active(void)
{
    char cmd[256];
    _snprintf(cmd, sizeof(cmd),
        "netsh advfirewall firewall show rule name=\"%s\" >nul 2>&1", RULE_NAME);
    return run_hidden(cmd) == 0; /* 0 = encontrou a regra */
}

/* ============================================================
 *  UI (desenho da janela)
 * ============================================================ */

static void make_fonts(void)
{
    #define MKF(h,w,b) CreateFontA(h,0,0,0,b,0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,\
        CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI")
    g_fBrand = MKF(20, 0, FW_BOLD);
    g_fTag   = MKF(13, 0, FW_NORMAL);
    g_fKick  = MKF(13, 0, FW_SEMIBOLD);
    g_fPill  = MKF(19, 0, FW_BOLD);
    g_fSub   = MKF(15, 0, FW_NORMAL);
    g_fBtn   = MKF(20, 0, FW_BOLD);
    g_fHelp  = MKF(14, 0, FW_NORMAL);
    g_fChip  = MKF(14, 0, FW_SEMIBOLD);
    g_fFoot  = MKF(13, 0, FW_NORMAL);
    #undef MKF
}

static void fill_round(HDC dc, RECT rc, int rad, COLORREF fill, COLORREF border)
{
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = border ? CreatePen(PS_SOLID, 1, border) : (HPEN)GetStockObject(NULL_PEN);
    HGDIOBJ ob = SelectObject(dc, br), op = SelectObject(dc, pn);
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, rad, rad);
    SelectObject(dc, ob); SelectObject(dc, op);
    DeleteObject(br); if (border) DeleteObject(pn);
}

static void draw_text(HDC dc, const char *s, RECT rc, HFONT f, COLORREF col, UINT fmt)
{
    HGDIOBJ of = SelectObject(dc, f);
    SetTextColor(dc, col); SetBkMode(dc, TRANSPARENT);
    DrawTextA(dc, s, -1, &rc, fmt);
    SelectObject(dc, of);
}

/* Desenha o logo GamingFlix (embutido) com transparencia, escalado para dx/dy. */
static void draw_logo(HDC dc, int dx, int dy, int dw, int dh)
{
    BITMAPINFO bi; ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = LOGO_W;
    bi.bmiHeader.biHeight      = -LOGO_H;   /* top-down */
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HDC mdc = CreateCompatibleDC(dc);
    HBITMAP dib = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (dib && bits) {
        memcpy(bits, LOGO_BGRA, sizeof(LOGO_BGRA));
        HGDIOBJ ob = SelectObject(mdc, dib);
        BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        SetStretchBltMode(dc, HALFTONE);
        AlphaBlend(dc, dx, dy, dw, dh, mdc, 0, 0, LOGO_W, LOGO_H, bf);
        SelectObject(mdc, ob);
    }
    if (dib) DeleteObject(dib);
    DeleteDC(mdc);
}

static void paint(HWND hwnd)
{
    PAINTSTRUCT ps; HDC wdc = BeginPaint(hwnd, &ps);
    RECT cr; GetClientRect(hwnd, &cr);
    int W = cr.right, H = cr.bottom;

    /* double buffer */
    HDC dc = CreateCompatibleDC(wdc);
    HBITMAP bm = CreateCompatibleBitmap(wdc, W, H);
    HGDIOBJ obm = SelectObject(dc, bm);

    /* fundo */
    HBRUSH bg = CreateSolidBrush(C_BG);
    RECT full = {0,0,W,H}; FillRect(dc, &full, bg); DeleteObject(bg);

    COLORREF accent   = g_offline ? C_GREEN   : C_AMBER;
    COLORREF accentHi = g_offline ? C_GREEN_Hi : C_AMBER_Hi;

    /* ---- title bar ---- */
    /* logo GamingFlix real (mantendo proporcao ~55x64) */
    {
        int lh = 30, lw = LOGO_W * lh / LOGO_H;  /* ~26px */
        draw_logo(dc, 18, 15, lw, lh);
    }
    RECT brand = {56, 15, 300, 45};
    draw_text(dc, "GAMINGFLIX", brand, g_fBrand, C_TEXT, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    /* destaque "FLIX" em vermelho por cima */
    {
        SIZE sz; HGDIOBJ of = SelectObject(dc, g_fBrand);
        GetTextExtentPoint32A(dc, "GAMING", 6, &sz);
        RECT rf = {56 + sz.cx, 15, 300, 45};
        SelectObject(dc, of);
        draw_text(dc, "FLIX", rf, g_fBrand, C_RED, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }
    /* tag "Ubisoft Offline" */
    {
        SIZE sz; HDC t = dc; HGDIOBJ of = SelectObject(t, g_fBrand);
        GetTextExtentPoint32A(t, "GAMINGFLIX", 10, &sz); SelectObject(t, of);
        RECT tag = {56 + sz.cx + 12, 20, 56 + sz.cx + 12 + 108, 40};
        fill_round(dc, tag, 10, C_BG, C_LINE);
        draw_text(dc, "Ubisoft Offline", tag, g_fTag, C_FAINT, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }
    /* botoes min / close (hit-rects) */
    SetRect(&r_min,   W-64, 20, W-46, 40);
    SetRect(&r_close, W-38, 20, W-18, 40);
    draw_text(dc, "\x97", r_min,   g_fBtn, C_MUTED, DT_CENTER|DT_VCENTER|DT_SINGLELINE); /* — */
    draw_text(dc, "\xD7", r_close, g_fBtn, C_MUTED, DT_CENTER|DT_VCENTER|DT_SINGLELINE); /* × */

    /* ---- kicker ---- */
    RECT kick = {0, 62, W, 84};
    draw_text(dc, "STATUS DA SUA UBISOFT", kick, g_fKick, C_MUTED, DT_CENTER|DT_SINGLELINE);

    /* ---- status card ---- */
    RECT card = {24, 92, W-24, 300};
    fill_round(dc, card, 16, C_CARD, C_LINE);

    /* ring */
    int cx = W/2, cy = 150, rad = 44;
    {
        HPEN pn = CreatePen(PS_SOLID, 3, accent);
        HBRUSH nb = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        HGDIOBJ op = SelectObject(dc, pn), ob = SelectObject(dc, nb);
        Ellipse(dc, cx-rad, cy-rad, cx+rad, cy+rad);
        SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pn);
        /* icone: check (offline/protegido) ou "-" (online/desprotegido) */
        HPEN ip = CreatePen(PS_SOLID, 4, accentHi);
        HGDIOBJ oip = SelectObject(dc, ip);
        if (g_offline) {
            MoveToEx(dc, cx-16, cy+2, NULL); LineTo(dc, cx-4, cy+14); LineTo(dc, cx+18, cy-12);
        } else {
            MoveToEx(dc, cx-14, cy, NULL); LineTo(dc, cx+14, cy);
        }
        SelectObject(dc, oip); DeleteObject(ip);
    }

    /* pill */
    {
        const char *label = g_offline ? "OFFLINE - PROTEGIDO" : "ONLINE - DESPROTEGIDO";
        SIZE sz; HGDIOBJ of = SelectObject(dc, g_fPill);
        GetTextExtentPoint32A(dc, label, (int)strlen(label), &sz);
        SelectObject(dc, of);
        int pw = sz.cx + 54, ph = 40;
        RECT pill = {cx - pw/2, 208, cx + pw/2, 208 + ph};
        fill_round(dc, pill, ph/2, C_CARD2, accent);
        /* dot */
        HBRUSH db = CreateSolidBrush(accent); HGDIOBJ ob = SelectObject(dc, db);
        HPEN np=(HPEN)GetStockObject(NULL_PEN); HGDIOBJ op=SelectObject(dc,np);
        Ellipse(dc, pill.left+16, 208+ph/2-5, pill.left+26, 208+ph/2+5);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(db);
        RECT pt = {pill.left+30, pill.top, pill.right, pill.bottom};
        draw_text(dc, label, pt, g_fPill, accentHi, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }

    /* sub */
    {
        const char *sub = g_offline
            ? "Ubisoft bloqueada no firewall. Pode jogar tranquilo."
            : "Sua conta pode cair se alguem logar online.";
        RECT rs = {36, 258, W-36, 290};
        draw_text(dc, sub, rs, g_fSub, C_MUTED, DT_CENTER|DT_WORDBREAK);
    }

    /* ---- primary button ---- */
    SetRect(&r_primary, 24, 320, W-24, 384);
    if (g_offline) {
        fill_round(dc, r_primary, 13, C_CARD, C_LINE);
        draw_text(dc, "Desativar modo offline", r_primary, g_fBtn, C_TEXT, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    } else {
        fill_round(dc, r_primary, 13, C_RED, C_RED_Hi);
        draw_text(dc, g_busy ? "Ativando..." : "Ativar modo offline",
                  r_primary, g_fBtn, RGB(255,255,255), DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }

    /* ---- help ---- */
    {
        const char *help = g_offline
            ? "Modo offline ativo. Pode abrir o jogo normalmente. Para voltar a atualizar/comprar na Ubisoft, desative aqui."
            : "Nao precisa abrir o jogo antes: ative aqui e depois abra a Ubisoft/jogo normal. Bloqueia a Ubisoft Connect no Firewall do Windows pra sua conta nao cair quando outra pessoa entra online.";
        RECT rh = {40, 394, W-40, 456};
        draw_text(dc, help, rh, g_fHelp, C_FAINT, DT_CENTER|DT_WORDBREAK);
    }

    /* ---- chips ---- */
    SetRect(&r_chipHow, 24, 466, W/2-5, 506);
    SetRect(&r_chipGit, W/2+5, 466, W-24, 506);
    fill_round(dc, r_chipHow, 11, C_CARD2, C_LINE);
    fill_round(dc, r_chipGit, 11, C_CARD2, C_LINE);
    draw_text(dc, "Como funciona", r_chipHow, g_fChip, C_MUTED, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    draw_text(dc, "GitHub",        r_chipGit, g_fChip, C_MUTED, DT_CENTER|DT_VCENTER|DT_SINGLELINE);

    /* ---- footer ---- */
    {
        HPEN pn = CreatePen(PS_SOLID, 1, C_LINE); HGDIOBJ op = SelectObject(dc, pn);
        MoveToEx(dc, 22, 524, NULL); LineTo(dc, W-22, 524);
        SelectObject(dc, op); DeleteObject(pn);
        RECT lf = {24, 530, W/2, 552};
        draw_text(dc, APP_VERSION " . gamingflix.space", lf, g_fFoot, C_FAINT, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        RECT rf = {W/2, 530, W-24, 552};
        draw_text(dc, "Codigo aberto", rf, g_fFoot, C_GREEN_Hi, DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
    }

    BitBlt(wdc, 0, 0, W, H, dc, 0, 0, SRCCOPY);
    SelectObject(dc, obm); DeleteObject(bm); DeleteDC(dc);
    EndPaint(hwnd, &ps);
}

static BOOL in_rect(RECT r, POINT p) { return PtInRect(&r, p); }

static void do_toggle(HWND hwnd)
{
    if (g_busy) return;
    g_busy = TRUE; InvalidateRect(hwnd, NULL, FALSE); UpdateWindow(hwnd);

    if (g_offline) {
        fw_unblock();
    } else {
        if (!fw_block()) {
            MessageBoxA(hwnd,
                "Nao encontrei a Ubisoft Connect instalada.\n"
                "Instale/abra a Ubisoft Connect uma vez e tente de novo.",
                APP_TITLE, MB_ICONWARNING);
        }
    }
    g_offline = fw_is_active();
    g_busy = FALSE;
    InvalidateRect(hwnd, NULL, FALSE);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        make_fonts();
        g_offline = fw_is_active();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        paint(hwnd);
        return 0;
    case WM_LBUTTONUP: {
        POINT p = { LOWORD(lp), HIWORD(lp) };
        if (in_rect(r_close, p)) { PostMessage(hwnd, WM_CLOSE, 0, 0); return 0; }
        if (in_rect(r_min, p))   { ShowWindow(hwnd, SW_MINIMIZE); return 0; }
        if (in_rect(r_primary, p)) { do_toggle(hwnd); return 0; }
        if (in_rect(r_chipHow, p)) { ShellExecuteA(hwnd,"open",REPO_URL"#como-funciona",NULL,NULL,SW_SHOW); return 0; }
        if (in_rect(r_chipGit, p)) { ShellExecuteA(hwnd,"open",REPO_URL,NULL,NULL,SW_SHOW); return 0; }
        return 0;
    }
    case WM_NCHITTEST: {
        /* permite arrastar a janela pela barra de titulo (sem moldura) */
        LRESULT hit = DefWindowProc(hwnd, msg, wp, lp);
        if (hit == HTCLIENT) {
            POINT p = { LOWORD(lp), HIWORD(lp) }; ScreenToClient(hwnd, &p);
            if (p.y < 56 && !in_rect(r_close, p) && !in_rect(r_min, p))
                return HTCAPTION;
        }
        return hit;
    }
    case WM_SETCURSOR: {
        POINT p; GetCursorPos(&p); ScreenToClient(hwnd, &p);
        if (in_rect(r_primary,p)||in_rect(r_chipHow,p)||in_rect(r_chipGit,p)||
            in_rect(r_close,p)||in_rect(r_min,p)) {
            SetCursor(LoadCursor(NULL, IDC_HAND)); return TRUE;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    (void)hPrev; (void)lpCmd;
    WNDCLASSA wc; ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "GFXUbiOffline";
    wc.hIcon         = LoadIconA(hInst, MAKEINTRESOURCEA(IDI_APPICON));
    RegisterClassA(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    /* janela sem moldura (WS_POPUP) com sombra fina */
    HWND hwnd = CreateWindowExA(
        WS_EX_APPWINDOW, "GFXUbiOffline", APP_TITLE,
        WS_POPUP | WS_MINIMIZEBOX,
        (sw-WIN_W)/2, (sh-WIN_H)/2, WIN_W, WIN_H,
        NULL, NULL, hInst, NULL);

    /* cantos arredondados na janela (Win11) e sombra */
    {
        HRGN rgn = CreateRoundRectRgn(0, 0, WIN_W+1, WIN_H+1, 18, 18);
        SetWindowRgn(hwnd, rgn, TRUE);
    }

    /* icone da janela (barra de tarefas + alt-tab) */
    {
        HICON hBig = (HICON)LoadImageA(hInst, MAKEINTRESOURCEA(IDI_APPICON),
                        IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
        HICON hSm  = (HICON)LoadImageA(hInst, MAKEINTRESOURCEA(IDI_APPICON),
                        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
        if (hBig) SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hBig);
        if (hSm)  SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hSm);
    }

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
