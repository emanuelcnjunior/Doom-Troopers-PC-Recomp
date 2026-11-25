#define NOMINMAX
#include <windows.h>

#include "menu_cheats.h"
#include "menu_ids.h"
#include "menu_skin.h"
#include <SDL2/SDL.h>
#include <bcrypt.h>
#include <commdlg.h>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <string>
#include <vector>
#include <wincrypt.h>

extern "C" int doom_runtime_main(int argc, char **argv);

namespace {
using std::wstring;
constexpr unsigned skip_levels = 1u << 6;
constexpr const wchar_t *section = L"doom_launcher";
constexpr const wchar_t *key_names[] = {L"key.B", L"key.C", L"key.A"};
constexpr const wchar_t *pad_names[] = {L"pad.B", L"pad.C", L"pad.A"};
constexpr int key_defaults[] = {SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_Z};
constexpr int pad_defaults[] = {2, 0, 1};

struct Menu {
    HWND window{};
    std::unique_ptr<MenuSkin> skin;
    wstring settings, launcher, rom;
    std::string runtime_path;
    HANDLE file = INVALID_HANDLE_VALUE;
    unsigned mask{};
    bool pt{}, exclusive_fullscreen{}, enhanced{}, enhanced_scale{}, audio_mods{}, linear{}, integer{}, vsync{true};
    int page{};
    SDL_GameController *controller{};
    const wchar_t *tr(const wchar_t *en, const wchar_t *br) const { return pt ? br : en; }
    HWND item(int id) const { return GetDlgItem(window, id); }
    void text(int id, const wchar_t *s) const { SetDlgItemTextW(window, id, s); }
    ~Menu() {
        if (file != INVALID_HANDLE_VALUE)
            CloseHandle(file);
    }
};

wstring read_ini(const wstring &file, const wchar_t *group, const wchar_t *key, const wchar_t *fallback) {
    wchar_t value[4096];
    GetPrivateProfileStringW(group, key, fallback, value, 4096, file.c_str());
    wstring s(value);
    auto end = s.find(L';');
    if (end != wstring::npos)
        s.erase(end);
    while (!s.empty() && iswspace(s.back()))
        s.pop_back();
    return s;
}

wstring wide(const std::string &s) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0);
    wstring out(n, 0);
    if (n)
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), &out[0], n);
    return out;
}

std::string narrow(const wstring &s, UINT cp) {
    BOOL loss = FALSE;
    BOOL *check = cp == CP_UTF8 ? nullptr : &loss;
    DWORD flags = check ? WC_NO_BEST_FIT_CHARS : WC_ERR_INVALID_CHARS;
    int n = WideCharToMultiByte(cp, flags, s.data(), (int)s.size(), nullptr, 0, nullptr, check);
    std::string out(n, 0);
    if (n)
        WideCharToMultiByte(cp, flags, s.data(), (int)s.size(), &out[0], n, nullptr, check);
    return loss ? std::string() : out;
}

wstring decode_path(const wstring &encoded) {
    DWORD size = 0;
    if (!CryptStringToBinaryW(encoded.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &size, nullptr, nullptr) ||
        !size)
        return {};
    std::string bytes(size, 0);
    if (!CryptStringToBinaryW(encoded.c_str(), 0, CRYPT_STRING_BASE64, (BYTE *)&bytes[0], &size, nullptr,
                              nullptr))
        return {};
    return wide(bytes);
}

wstring encode_path(const wstring &path) {
    auto bytes = narrow(path, CP_UTF8);
    DWORD size = 0, flags = CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF;
    if (!CryptBinaryToStringW((BYTE *)bytes.data(), (DWORD)bytes.size(), flags, nullptr, &size))
        return {};
    wstring out(size, 0);
    if (!CryptBinaryToStringW((BYTE *)bytes.data(), (DWORD)bytes.size(), flags, &out[0], &size))
        return {};
    out.resize(size);
    return out;
}

// Keep the verified file open without write/delete sharing through the runtime call.
bool verify_rom(Menu &m) {
    wchar_t path[2048], absolute[2048];
    GetDlgItemTextW(m.window, ID_ROM, path, 2048);
    DWORD len = GetFullPathNameW(path, 2048, absolute, nullptr);
    if (!*path || !len || len >= 2048)
        return false;
    m.rom = absolute;
    m.runtime_path = narrow(m.rom, GetACP());
    if (m.runtime_path.empty()) {
        DWORD n = GetShortPathNameW(absolute, path, 2048);
        if (n && n < 2048)
            m.runtime_path = narrow(path, GetACP());
    }
    if (m.runtime_path.empty())
        return false;
    if (m.file != INVALID_HANDLE_VALUE)
        CloseHandle(m.file);
    m.file = CreateFileW(absolute, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                         nullptr);
    if (m.file == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER size;
    std::vector<BYTE> bytes(2097152);
    DWORD count = 0;
    BYTE digest[32];
    constexpr BYTE expected[] = {0xd2, 0xfe, 0xdd, 0x7f, 0x02, 0x45, 0x71, 0xa4, 0xa3, 0x0d, 0x24,
                                 0x1b, 0xca, 0x2d, 0x66, 0xfc, 0xd5, 0xdb, 0x60, 0x28, 0x39, 0x14,
                                 0xc7, 0x92, 0xc6, 0x11, 0xf6, 0x88, 0xb8, 0x3a, 0xd3, 0xa2};
    bool ok =
        GetFileSizeEx(m.file, &size) && size.QuadPart == bytes.size() &&
        ReadFile(m.file, bytes.data(), (DWORD)bytes.size(), &count, nullptr) && count == bytes.size() &&
        BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0, bytes.data(), count, digest, sizeof digest) == 0 &&
        memcmp(digest, expected, sizeof digest) == 0;
    if (!ok) {
        CloseHandle(m.file);
        m.file = INVALID_HANDLE_VALUE;
    }
    return ok;
}

int selection(HWND control) { return (int)SendMessageW(control, CB_GETCURSEL, 0, 0); }
int choice(HWND control) { return (int)SendMessageW(control, CB_GETITEMDATA, selection(control), 0); }
void add_choice(HWND control, const wchar_t *text, int value, bool selected) {
    auto i = SendMessageW(control, CB_ADDSTRING, 0, (LPARAM)text);
    SendMessageW(control, CB_SETITEMDATA, i, value);
    if (selected)
        SendMessageW(control, CB_SETCURSEL, i, 0);
}

void cheat_list(Menu &m, int index) {
    HWND list = m.item(ID_CHEAT_LIST);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (unsigned i = 0; i < 30; ++i) {
        wstring line = m.mask & (1u << i) ? L"[x] " : L"[ ] ";
        line += cheat_names[i][m.pt];
        if (i == 6)
            line += m.tr(L" (experimental, disabled)", L" (experimental, desabilitado)");
        SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)line.c_str());
    }
    SendMessageW(list, LB_SETCURSEL, index, 0);
    EnableWindow(m.item(ID_TOGGLE), index != 6);
}

void load_profile(Menu &m) {
    m.linear = read_ini(m.settings, L"video", L"linear_filter", L"0") == L"1";
    m.integer = read_ini(m.launcher, section, L"integer_scale", L"0") == L"1";
    m.vsync = read_ini(m.launcher, section, L"vsync", L"1") != L"0";
    m.audio_mods = read_ini(m.launcher, section, L"audio_mods", L"0") == L"1";
    auto mode = read_ini(m.launcher, section, L"profile", L"");
    auto legacy = read_ini(m.launcher, section, L"scale2x", L"");
    m.enhanced = mode == L"enhanced" || (mode.empty() && legacy == L"1");
    m.enhanced_scale = read_ini(m.launcher, section, L"enhanced_scale2x",
                                legacy.c_str()) == L"1";
}

void show_profile(Menu &m) {
    CheckRadioButton(m.window, ID_CLASSIC, ID_ENHANCED, m.enhanced ? ID_ENHANCED : ID_CLASSIC);
    CheckDlgButton(m.window, ID_SCALE, m.enhanced && m.enhanced_scale ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m.window, ID_AUDIO_MODS, m.enhanced && m.audio_mods ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(m.item(ID_AUDIO_MODS), m.enhanced);
    CheckDlgButton(m.window, ID_LINEAR, m.enhanced && m.linear ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m.window, ID_INTEGER, m.enhanced && m.integer ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m.window, ID_VSYNC, (!m.enhanced || m.vsync) ? BST_CHECKED : BST_UNCHECKED);
    for (int id : {ID_LINEAR, ID_INTEGER, ID_VSYNC})
        EnableWindow(m.item(id), m.enhanced);
    EnableWindow(m.item(ID_SCALE), m.enhanced);
}

void apply_profile(const Menu &m) {
    _putenv_s("DOOM_LINEAR", m.enhanced && m.linear ? "1" : "0");
    _putenv_s("DOOM_INTEGER", m.enhanced && m.integer ? "1" : "0");
    _putenv_s("DOOM_VSYNC", !m.enhanced || m.vsync ? "1" : "0");
    _putenv_s("DOOM_AUDIO_MODS", m.enhanced && m.audio_mods ? "1" : "0");
    _putenv_s("DOOM_CLASSIC", m.enhanced ? "0" : "1");
    _putenv_s("DOOM_SCALE2X", m.enhanced && m.enhanced_scale ? "1" : "0");
}

void refresh(Menu &m) {
    m.text(ID_GAME, m.tr(L"Game", L"Jogo"));
    m.text(ID_CONTROLS, m.tr(L"Controls", L"Controles"));
    m.text(ID_ROM_LABEL, m.tr(L"Genesis / Mega Drive ROM", L"ROM do Mega Drive"));
    m.text(ID_LANG_LABEL, m.tr(L"Language", L"Idioma"));
    m.text(ID_LINEAR, m.tr(L"Bilinear filtering", L"Filtro bilinear"));
    m.text(ID_INTEGER, m.tr(L"Integer scaling (4:3)", L"Escala inteira (4:3)"));
    m.text(ID_VSYNC, m.tr(L"Request VSync", L"Solicitar VSync"));
    m.text(ID_AUDIO_MODS, m.tr(L"Use custom audio", L"Usar arquivos de som personalizados"));
    m.text(ID_CLASSIC, m.tr(L"Classic", L"Clássico"));
    m.text(ID_ENHANCED, m.tr(L"Enhanced", L"Aprimorado"));
    m.text(ID_SCALE, m.tr(L"Scale2x filter", L"Filtro Scale2x"));
    m.text(ID_DEFAULTS, m.tr(L"Restore defaults", L"Restaurar padrão"));
    m.text(ID_GAME_HEADING, m.tr(L"Game", L"Jogo"));
    m.text(ID_GAME_INFO, m.tr(L"Select your Doom Troopers (USA) ROM below. Play verifies the original 2 MiB "
                              L"cartridge image before starting.",
                              L"Selecione abaixo sua ROM de Doom Troopers (USA). Jogar verifica a imagem "
                              L"original de 2 MiB antes de iniciar."));
    m.text(ID_BROWSE, m.tr(L"Browse…", L"Procurar…"));
    m.text(ID_FULL, m.tr(L"Fullscreen", L"Tela cheia"));
    m.text(IDOK, m.tr(L"Play", L"Jogar"));
    m.text(ID_ACTION, m.tr(L"Player 1", L"Jogador 1"));
    m.text(ID_KEY_LABEL, m.tr(L"Keyboard", L"Teclado"));
    m.text(ID_SHOOT, m.tr(L"Shoot", L"Atirar"));
    m.text(ID_JUMP, m.tr(L"Jump", L"Pular"));
    m.text(ID_MELEE, m.tr(L"Melee attack", L"Atacar"));
    m.text(ID_SAVE_HINT, m.tr(L"L2   Save state", L"L2   Salvar partida"));
    m.text(ID_LOAD_HINT, m.tr(L"R2   Load state", L"R2   Carregar partida"));
    const wchar_t *buttons[][2] = {{L"X (cross)", L"X (cruz)"},
                                   {L"Circle", L"Bolinha"},
                                   {L"Square", L"Quadrado"},
                                   {L"Triangle", L"Triângulo"}};
    for (int i = 0; i < 3; ++i) {
        HWND control = m.item(ID_PAD0 + i);
        int selected = selection(control);
        // Keep any fifth, custom binding imported from settings.ini in its original slot.
        for (int b = 0; b < 4; ++b) {
            SendMessageW(control, CB_DELETESTRING, b, 0);
            SendMessageW(control, CB_INSERTSTRING, b, (LPARAM)buttons[b][m.pt]);
            SendMessageW(control, CB_SETITEMDATA, b, b);
        }
        SendMessageW(control, CB_SETCURSEL, selected, 0);
    }
    m.text(ID_TOGGLE, m.tr(L"Toggle", L"Ativar/desativar"));
    m.text(ID_CHEAT_HINT, m.tr(L"Individual cheat effects are still under test.",
                               L"Os efeitos individuais dos cheats ainda estão em teste."));
    cheat_list(m, 0);
}

void page(Menu &m, int tab) {
    m.page = tab;
    for (int id = ID_ACTION; id <= ID_CHEAT_HINT; ++id)
        ShowWindow(m.item(id), tab == (id <= ID_SAVE_HINT ? 1 : 2) ? SW_SHOW : SW_HIDE);
    for (int id : {ID_DEFAULTS, ID_LOAD_HINT})
        ShowWindow(m.item(id), tab == 1 ? SW_SHOW : SW_HIDE);
    for (int id : {ID_GAME_HEADING, ID_GAME_INFO, ID_CLASSIC, ID_ENHANCED, ID_AUDIO_MODS, ID_LINEAR, ID_INTEGER, ID_VSYNC})
        ShowWindow(m.item(id), tab == 0 ? SW_SHOW : SW_HIDE);
    for (int id = ID_GAME; id <= ID_CHEATS; ++id)
        InvalidateRect(m.item(id), nullptr, FALSE);
}

void init(Menu &m) {
    m.pt = read_ini(m.launcher, section, L"language", L"en") == L"pt";
    add_choice(m.item(ID_LANGUAGE), L"English", 0, !m.pt);
    add_choice(m.item(ID_LANGUAGE), L"Português", 1, m.pt);
    m.text(ID_ROM, decode_path(read_ini(m.launcher, section, L"rom_utf8", L"")).c_str());
    auto fs = read_ini(m.settings, L"video", L"fullscreen", L"1");
    m.exclusive_fullscreen = fs == L"2";
    CheckDlgButton(m.window, ID_FULL, fs == L"0" ? BST_UNCHECKED : BST_CHECKED);
    load_profile(m);
    show_profile(m);
    m.mask = wcstoul(read_ini(m.launcher, section, L"cheats", L"0").c_str(), nullptr, 10) & ((1u << 30) - 1) &
             ~skip_levels;
    const wchar_t *buttons[] = {L"×", L"○", L"□", L"△"};
    for (int i = 0; i < 3; ++i) {
        int key = _wtoi(
            read_ini(m.settings, L"input.p1", key_names[i], std::to_wstring(key_defaults[i]).c_str()).c_str());
        auto pad = read_ini(m.settings, L"input.p1", pad_names[i],
                            (L"button:" + std::to_wstring(pad_defaults[i])).c_str());
        for (int k = 1; k < SDL_NUM_SCANCODES; ++k) {
            auto name = wide(SDL_GetScancodeName((SDL_Scancode)k));
            if (!name.empty())
                add_choice(m.item(ID_KEY0 + i), name.c_str(), k, k == key);
        }
        if (selection(m.item(ID_KEY0 + i)) < 0)
            add_choice(m.item(ID_KEY0 + i), std::to_wstring(key).c_str(), key, true);
        for (int b = 0; b < 4; ++b)
            add_choice(m.item(ID_PAD0 + i), buttons[b], b, pad == L"button:" + std::to_wstring(b));
        if (selection(m.item(ID_PAD0 + i)) < 0)
            add_choice(m.item(ID_PAD0 + i), pad.c_str(), -1, true);
    }
    refresh(m);
    page(m, 0);
}

void warn(Menu &m, const wchar_t *en, const wchar_t *pt) {
    MessageBoxW(m.window, m.tr(en, pt), L"Doom Troopers", MB_ICONWARNING | MB_OK);
}

bool save(Menu &m) {
    int keys[3], pads[3];
    for (int i = 0; i < 3; ++i) {
        keys[i] = choice(m.item(ID_KEY0 + i));
        pads[i] = choice(m.item(ID_PAD0 + i));
        for (int j = 0; j < i; ++j)
            if (keys[i] == keys[j] || (pads[i] >= 0 && pads[i] == pads[j])) {
                page(m, 1);
                warn(m, L"Choose distinct keys and buttons.", L"Escolha teclas e botões distintos.");
                return false;
            }
    }
    if (!verify_rom(m)) {
        page(m, 0);
        warn(m, L"Select the original USA ROM at a readable path (size and SHA-256 required).",
             L"Selecione a ROM USA original em um caminho legível (tamanho e SHA-256 obrigatórios).");
        return false;
    }
    auto put = [](const wstring &path, const wchar_t *group, const wchar_t *key, const wstring &value) {
        return WritePrivateProfileStringW(group, key, value.c_str(), path.c_str()) != 0;
    };
    bool scale = IsDlgButtonChecked(m.window, ID_SCALE) == BST_CHECKED;
    auto encoded = encode_path(m.rom);
    bool ok = !encoded.empty() && put(m.launcher, section, L"rom_utf8", encoded) &&
              put(m.launcher, section, L"enhanced_scale2x", m.enhanced_scale ? L"1" : L"0") &&
              put(m.settings, L"video", L"linear_filter", m.linear ? L"1" : L"0") &&
              put(m.launcher, section, L"integer_scale", m.integer ? L"1" : L"0") &&
              put(m.launcher, section, L"vsync", m.vsync ? L"1" : L"0") &&
              put(m.launcher, section, L"audio_mods", m.audio_mods ? L"1" : L"0") &&
              put(m.launcher, section, L"profile", m.enhanced ? L"enhanced" : L"classic") &&
              put(m.launcher, section, L"scale2x", scale ? L"1" : L"0") &&
              put(m.launcher, section, L"language", m.pt ? L"pt" : L"en") &&
              put(m.launcher, section, L"cheats", std::to_wstring(m.mask)) &&
              put(m.settings, L"video", L"fullscreen",
                  IsDlgButtonChecked(m.window, ID_FULL) ? (m.exclusive_fullscreen ? L"2" : L"1") : L"0");
    for (int i = 0; ok && i < 3; ++i)
        ok = put(m.settings, L"input.p1", key_names[i], std::to_wstring(keys[i])) &&
             (pads[i] < 0 || put(m.settings, L"input.p1", pad_names[i], L"button:" + std::to_wstring(pads[i])));
    if (!ok) {
        warn(m, L"Could not save settings in the game folder.",
             L"Falha ao salvar as preferências na pasta do jogo.");
        return false;
    }
    apply_profile(m);
    _putenv_s("DOOM_CHEAT_MASK", std::to_string(m.mask).c_str());
    _putenv_s("GENESIS_FORCE_INTERP", (m.mask & ~0x103fu) ? "1" : "0");
    _putenv_s("GENESIS_STRICT_JSR_STACK", "1");
    return true;
}

void toggle(Menu &m, int index) {
    if (index < 0 || index >= 30 || index == 6)
        return;
    unsigned bit = 1u << index;
    if (!(m.mask & bit)) {
        for (unsigned group : {0xC0u, 0x1E0000u, 0x600000u, 0x1800000u, 0x38000000u})
            if (group & bit)
                m.mask &= ~group;
    }
    m.mask ^= bit;
    cheat_list(m, index);
}

void controller_input(Menu &m) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_CONTROLLERDEVICEADDED && !m.controller)
            m.controller = SDL_GameControllerOpen(e.cdevice.which);
        if (e.type == SDL_CONTROLLERDEVICEREMOVED && m.controller &&
            !SDL_GameControllerGetAttached(m.controller)) {
            SDL_GameControllerClose(m.controller);
            m.controller = nullptr;
        }
        if (e.type != SDL_CONTROLLERBUTTONDOWN || GetActiveWindow() != m.window)
            continue;
        HWND focus = GetFocus();
        int id = GetDlgCtrlID(focus), button = e.cbutton.button;
        bool combo = id == ID_LANGUAGE || (id >= ID_KEY0 && id <= ID_PAD2);
        if (button == SDL_CONTROLLER_BUTTON_START)
            PostMessageW(m.window, WM_COMMAND, IDOK, 0);
        else if (button == SDL_CONTROLLER_BUTTON_A) {
            if (id == ID_CHEAT_LIST)
                SendMessageW(m.window, WM_COMMAND, ID_TOGGLE, 0);
            else if (!combo && id != ID_ROM)
                SendMessageW(focus, BM_CLICK, 0, 0);
        } else if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT || button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
            int d = button == SDL_CONTROLLER_BUTTON_DPAD_LEFT ? -1 : 1;
            if (combo) {
                int n = (int)SendMessageW(focus, CB_GETCOUNT, 0, 0);
                SendMessageW(focus, CB_SETCURSEL, (selection(focus) + d + n) % n, 0);
                if (id == ID_LANGUAGE) {
                    m.pt = selection(focus) == 1;
                    refresh(m);
                }
            } else if (id == ID_CHEAT_LIST) {
                int row = (int)SendMessageW(focus, LB_GETCURSEL, 0, 0);
                cheat_list(m, (row + d + 30) % 30);
            }
        } else if (button == SDL_CONTROLLER_BUTTON_DPAD_UP || button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
            SendMessageW(m.window, WM_NEXTDLGCTL, button == SDL_CONTROLLER_BUTTON_DPAD_UP, 0);
    }
}

INT_PTR CALLBACK dialog(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    auto m = reinterpret_cast<Menu *>(GetWindowLongPtrW(w, DWLP_USER));
    if (msg == WM_INITDIALOG) {
        m = reinterpret_cast<Menu *>(lp);
        m->window = w;
        SetWindowLongPtrW(w, DWLP_USER, lp);
        SendMessageW(w, WM_SETICON, ICON_BIG,
                     (LPARAM)LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1)));
        m->skin = std::make_unique<MenuSkin>();
        m->skin->open(w);
        init(*m);
        SetTimer(w, 1, 40, nullptr);
        return TRUE;
    }
    if (!m)
        return FALSE;
    if (msg == WM_ERASEBKGND) {
        m->skin->background(w, (HDC)wp);
        return TRUE;
    }
    if (msg == WM_CTLCOLORDLG || msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT ||
        msg == WM_CTLCOLORLISTBOX || msg == WM_CTLCOLORBTN)
        return m->skin->colors((HWND)lp, (HDC)wp, msg);
    if (msg == WM_DRAWITEM) {
        m->skin->draw(*reinterpret_cast<DRAWITEMSTRUCT *>(lp), m->page);
        return TRUE;
    }
    if (msg == WM_TIMER) {
        controller_input(*m);
        return TRUE;
    }
    if (msg == WM_CLOSE) {
        EndDialog(w, IDCANCEL);
        return TRUE;
    }
    if (msg != WM_COMMAND)
        return FALSE;
    int id = LOWORD(wp), event = HIWORD(wp);
    if (id >= ID_GAME && id <= ID_CHEATS)
        page(*m, id - ID_GAME);
    else if ((id == ID_CLASSIC || id == ID_ENHANCED) && event == BN_CLICKED) {
        m->enhanced = id == ID_ENHANCED;
        show_profile(*m);
    } else if (id == ID_LINEAR && event == BN_CLICKED) {
        m->linear = IsDlgButtonChecked(w, id) == BST_CHECKED;
    } else if (id == ID_INTEGER && event == BN_CLICKED) {
        m->integer = IsDlgButtonChecked(w, id) == BST_CHECKED;
    } else if (id == ID_VSYNC && event == BN_CLICKED) {
        m->vsync = IsDlgButtonChecked(w, id) == BST_CHECKED;
    } else if (id == ID_AUDIO_MODS && event == BN_CLICKED) {
        m->audio_mods = IsDlgButtonChecked(w, ID_AUDIO_MODS) == BST_CHECKED;
    } else if (id == ID_SCALE && event == BN_CLICKED) {
        m->enhanced_scale = IsDlgButtonChecked(w, ID_SCALE) == BST_CHECKED;
    } else if (id == ID_DEFAULTS) {
        for (int i = 0; i < 3; ++i) {
            HWND key = m->item(ID_KEY0 + i);
            int count = (int)SendMessageW(key, CB_GETCOUNT, 0, 0);
            for (int row = 0; row < count; ++row)
                if (SendMessageW(key, CB_GETITEMDATA, row, 0) == key_defaults[i]) {
                    SendMessageW(key, CB_SETCURSEL, row, 0);
                    break;
                }
            SendMessageW(m->item(ID_PAD0 + i), CB_SETCURSEL, pad_defaults[i], 0);
        }
    } else if (id == ID_LANGUAGE && event == CBN_SELCHANGE) {
        m->pt = selection(m->item(id)) == 1;
        refresh(*m);
    } else if (id == ID_BROWSE) {
        wchar_t path[2048] = {};
        OPENFILENAMEW f = {sizeof(f)};
        f.hwndOwner = w;
        f.lpstrFile = path;
        f.nMaxFile = 2048;
        f.lpstrFilter = L"Doom Troopers (USA)\0*.md;*.bin;*.gen\0*.*\0*.*\0";
        f.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameW(&f))
            m->text(ID_ROM, path);
    } else if (id == ID_CHEAT_LIST || id == ID_TOGGLE) {
        int row = (int)SendMessageW(m->item(ID_CHEAT_LIST), LB_GETCURSEL, 0, 0);
        if (id == ID_TOGGLE || event == LBN_DBLCLK)
            toggle(*m, row);
        else
            EnableWindow(m->item(ID_TOGGLE), row >= 0 && row != 6);
    } else if (id == IDOK) {
        if (save(*m))
            EndDialog(w, IDOK);
    } else if (id == IDCANCEL)
        EndDialog(w, IDCANCEL);
    else
        return FALSE;
    return TRUE;
}
} // namespace

int main(int argc, char **argv) {
    wchar_t path[2048];
    DWORD size = GetModuleFileNameW(nullptr, path, 2048);
    if (!size || size >= 2048)
        return 1;
    wstring folder(path);
    folder.resize(folder.find_last_of(L"\\/") + 1);
    Menu menu;
    menu.settings = folder + L"settings.ini";
    menu.launcher = folder + L"launcher.ini";
    if (argc != 1 || getenv("GENESIS_NO_LAUNCHER") != nullptr) {
        load_profile(menu);
        const char *override_scale = getenv("DOOM_SCALE2X");
        std::string scale = override_scale ? override_scale : "";
        apply_profile(menu);
        if (!scale.empty())
            _putenv_s("DOOM_SCALE2X", scale.c_str());
        const char *value = getenv("DOOM_CHEAT_MASK");
        if (value) {
            char *end;
            unsigned long mask = strtoul(value, &end, 10);
            if (*value && !*end && mask < (1ul << 30) && (mask & skip_levels))
                _putenv_s("DOOM_CHEAT_MASK", std::to_string(mask & ~skip_levels).c_str());
        }
        return doom_runtime_main(argc, argv);
    }
    bool input = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
    INT_PTR result =
        DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(ID_MENU), nullptr, dialog, (LPARAM)&menu);
    menu.skin.reset();
    if (menu.controller)
        SDL_GameControllerClose(menu.controller);
    if (input)
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    if (result != IDOK)
        return result == -1 ? 1 : 0;
    char *args[] = {argv[0], menu.runtime_path.data(), nullptr};
    return doom_runtime_main(2, args);
}
