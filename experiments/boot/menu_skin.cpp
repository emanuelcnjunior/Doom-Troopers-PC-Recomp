#define NOMINMAX
#include "menu_skin.h"
#include "menu_ids.h"
#include <algorithm>
#include <shlwapi.h>

namespace {
constexpr COLORREF dark = RGB(16, 22, 29), panel = RGB(27, 37, 46), amber = RGB(255, 178, 76);
constexpr COLORREF ink = RGB(235, 239, 234), muted = RGB(168, 185, 192);
void fill(HDC dc, RECT bounds, COLORREF color) {
    SetDCBrushColor(dc, color);
    FillRect(dc, &bounds, (HBRUSH)GetStockObject(DC_BRUSH));
}
} // namespace

void MenuSkin::open(HWND w) {
    night = CreateSolidBrush(dark);
    surface = CreateSolidBrush(panel);
    gold = CreateSolidBrush(amber);
    LOGFONTW lf{};
    GetObjectW((HFONT)SendMessageW(w, WM_GETFONT, 0, 0), sizeof lf, &lf);
    lf.lfHeight = MulDiv(lf.lfHeight, 115, 100);
    normal = CreateFontIndirectW(&lf);
    for (HWND child = GetWindow(w, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
        SendMessageW(child, WM_SETFONT, (WPARAM)normal, FALSE);
    lf.lfWeight = FW_BOLD;
    bold = CreateFontIndirectW(&lf);
    for (int id : {ID_SAVE_HINT, ID_LOAD_HINT, ID_REGION, ID_LANG_LABEL, ID_ROM_LABEL, ID_GAME_HEADING, IDOK})
        SendDlgItemMessageW(w, id, WM_SETFONT, (WPARAM)bold, FALSE);
    Gdiplus::GdiplusStartupInput options;
    if (Gdiplus::GdiplusStartup(&token, &options, nullptr) != Gdiplus::Ok)
        return;
    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(ID_COVER), MAKEINTRESOURCEW(10));
    if (resource) {
        auto bytes = static_cast<const BYTE *>(LockResource(LoadResource(module, resource)));
        if (bytes)
            stream = SHCreateMemStream(bytes, SizeofResource(module, resource));
        if (stream)
            cover.reset(Gdiplus::Image::FromStream(stream));
    }
}
MenuSkin::~MenuSkin() {
    cover.reset();
    if (stream)
        stream->Release();
    if (token)
        Gdiplus::GdiplusShutdown(token);
    for (auto b : {night, surface, gold})
        if (b)
            DeleteObject(b);
    if (bold)
        DeleteObject(bold);
    if (normal)
        DeleteObject(normal);
}
void MenuSkin::background(HWND w, HDC dc) const {
    RECT r;
    GetClientRect(w, &r);
    FillRect(dc, &r, night);
    RECT sidebar = {0, 0, 120, 288}, line = {132, 190, 444, 191};
    MapDialogRect(w, &sidebar);
    MapDialogRect(w, &line);
    FillRect(dc, &sidebar, surface);
    FillRect(dc, &line, surface);
}
INT_PTR MenuSkin::colors(HWND control, HDC dc, UINT message) const {
    int id = GetDlgCtrlID(control);
    bool input = message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX || id == ID_LANGUAGE ||
                 (id >= ID_KEY0 && id <= ID_PAD2);
    bool rail = id == ID_REGION || id == ID_LANG_LABEL;
    COLORREF color = ink;
    if (id == ID_SAVE_HINT || id == ID_LOAD_HINT)
        color = amber;
    else if (rail || id == ID_ROM_LABEL || id == ID_ACTION || id == ID_KEY_LABEL || id == ID_PAD_LABEL ||
             id == ID_CHEAT_HINT)
        color = muted;
    SetTextColor(dc, color);
    SetBkColor(dc, input || rail ? panel : dark);
    return (INT_PTR)(input || rail ? surface : night);
}
void MenuSkin::draw(const DRAWITEMSTRUCT &d, int page) const {
    RECT r = d.rcItem;
    int id = d.CtlID;
    bool nav = id >= ID_GAME && id <= ID_CHEATS;
    bool active = id == IDOK || (nav && id == ID_GAME + page);
    HDC dc = d.hDC;
    int state = SaveDC(dc);
    wchar_t label[128] = {};
    GetWindowTextW(d.hwndItem, label, 128);
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, (HFONT)SendMessageW(d.hwndItem, WM_GETFONT, 0, 0));
    int height = r.bottom - r.top;
    if (id == ID_TITLE) {
        FillRect(dc, &r, night);
        if (token) {
            using namespace Gdiplus;
            Graphics g(dc);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            FontFamily face(L"Georgia");
            GraphicsPath path;
            path.AddString(label, -1, &face, FontStyleBold, height * 0.67f, PointF(0, 0),
                           StringFormat::GenericTypographic());
            Pen stroke(Color(125, 48, 30), height * .046f);
            LinearGradientBrush gradient(Point(0, 0), Point(0, height), Color(255, 236, 165),
                                         Color(230, 148, 46));
            g.DrawPath(&stroke, &path);
            g.FillPath(&gradient, &path);
        } else {
            SetTextColor(dc, amber);
            DrawTextW(dc, label, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        fill(dc, {r.left, r.bottom - 3, r.right, r.bottom}, RGB(166, 52, 44));
    } else if (id == ID_GAME) {
        FillRect(dc, &r, surface);
        RECT art = r;
        art.bottom -= MulDiv(height, 9, 100);
        fill(dc, art, active ? amber : RGB(164, 118, 62));
        InflateRect(&art, -2, -2);
        fill(dc, art, dark);
        if (cover && cover->GetLastStatus() == Gdiplus::Ok && cover->GetWidth() && cover->GetHeight()) {
            float scale = std::min((art.right - art.left) / (float)cover->GetWidth(),
                                   (art.bottom - art.top) / (float)cover->GetHeight());
            int width = (int)(cover->GetWidth() * scale + .5f), h = (int)(cover->GetHeight() * scale + .5f);
            Gdiplus::Graphics g(dc);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g.DrawImage(cover.get(), (art.left + art.right - width) / 2, (art.top + art.bottom - h) / 2, width,
                        h);
        }
        RECT caption = {r.left, art.bottom + 2, r.right, r.bottom};
        SetTextColor(dc, active ? amber : muted);
        DrawTextW(dc, label, -1, &caption, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        FillRect(dc, &r, active ? gold : surface);
        if (d.itemState & ODS_SELECTED) {
            fill(dc, r, active ? RGB(235, 155, 58) : RGB(44, 57, 68));
            OffsetRect(&r, 0, 1);
        }
        SetTextColor(dc, (d.itemState & ODS_DISABLED) ? muted : active ? dark : ink);
        DrawTextW(dc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    if ((d.itemState & ODS_FOCUS) && !(d.itemState & ODS_NOFOCUSRECT)) {
        RECT focus = d.rcItem;
        InflateRect(&focus, -3, -3);
        DrawFocusRect(dc, &focus);
    }
    RestoreDC(dc, state);
}
